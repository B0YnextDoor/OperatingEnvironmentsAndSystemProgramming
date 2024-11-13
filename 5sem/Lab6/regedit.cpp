#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

class Logger
{
	std::fstream logFile;
	std::fstream backupFile;
	std::string logPath;
	std::ios_base::openmode logOpen = std::ios::app | std::ios::ate;
	std::ios_base::openmode backupOpen = std::ios::in | std::ios::app | std::ios::ate;

public:
	HWND logEdit = nullptr;
	std::string backupPath;

	Logger(const std::string &fileLog, const std::string &fileBackUp) : logPath(fileLog), backupPath(fileBackUp)
	{
		logFile.open(fileLog, logOpen);
		backupFile.open(fileBackUp, backupOpen);
	}

	~Logger()
	{
		close();
	}

	void close()
	{
		if (logFile.is_open())
			logFile.close();
		if (backupFile.is_open())
			backupFile.close();
	}

	void AppendToLog(const char *text)
	{
		if (!logEdit)
			return;
		int len = GetWindowTextLength(logEdit);
		SendMessage(logEdit, EM_SETSEL, len, len);
		SendMessage(logEdit, EM_REPLACESEL, 0, (LPARAM)text);
		SendMessage(logEdit, EM_REPLACESEL, 0, (LPARAM) "\r\n");
	}

	bool log(const std::string &message)
	{
		try
		{
			if (!logFile.is_open())
				logFile.open(logPath, logOpen);
			logFile << message << "\n";
			std::cout << message << "\n";
			AppendToLog(message.c_str());
			return 0;
		}
		catch (...)
		{
			return 1;
		}
	}

	bool backup(const std::string &fullPath, const std::string &valueName, const std::string &valueData)
	{
		try
		{
			if (!backupFile.is_open())
				backupFile.open(backupPath);
			backupFile << fullPath << "\\" << valueName << "=" << valueData << "\n";
			return 0;
		}
		catch (...)
		{
			return 1;
		}
	}
};

class RegEdit
{
private:
	Logger *logger;

	bool isFileExists(const std::string &filePath)
	{
		return PathFileExistsA(filePath.c_str()) == TRUE;
	}

	bool deleteKey(HKEY key, const std::string &subKey)
	{
		char valueName[MAX_PATH];
		DWORD valueNameSize;
		BYTE valueData[4096];
		DWORD valueDataSize;
		DWORD valueType;
		DWORD index = 0;
		std::vector<std::string> valuesToDelete;

		while (true)
		{
			valueNameSize = MAX_PATH;
			valueDataSize = sizeof(valueData);

			LONG result = RegEnumValueA(key, index, valueName, &valueNameSize, nullptr,
										&valueType, valueData, &valueDataSize);

			if (result == ERROR_NO_MORE_ITEMS)
				break;

			if (result == ERROR_SUCCESS)
			{
				if (valueType == REG_SZ)
				{
					std::string filePath(reinterpret_cast<char *>(valueData));
					std::string key = subKey + "\\" + std::string(valueName) + " -> " + filePath;
					if (!isFileExists(filePath))
					{
						if (logger->backup(subKey, valueName, filePath))
							return 1;
						logger->log("Found non-actual key: " + key);
						valuesToDelete.push_back(valueName);
					}
					else
						logger->log("Key --- " + key + " --- is actual.");
				}
			}
			index++;
		}
		for (const auto &valueToDelete : valuesToDelete)
		{
			LONG deleteResult = RegDeleteValueA(key, valueToDelete.c_str());
			if (deleteResult == ERROR_SUCCESS)
				logger->log("Successfully deleted key: " + subKey + "\\" + valueToDelete);
			else
				logger->log("Failed to delete key: " + subKey + "\\" + valueToDelete);
		}
		return 0;
	}

public:
	explicit RegEdit(Logger *logger) : logger(logger) {}

	bool cleanRegistry(HKEY hKey, const std::string &subKey)
	{
		HKEY key;
		if (RegOpenKeyExA(hKey, subKey.c_str(), 0, KEY_READ | KEY_WRITE, &key) != ERROR_SUCCESS)
		{
			logger->log("Failed to open key: " + subKey);
			return 1;
		}

		char subKeyName[MAX_PATH];
		DWORD subKeyNameSize;
		DWORD index = 0;

		while (true)
		{
			subKeyNameSize = MAX_PATH;
			LONG result = RegEnumKeyExA(key, index, subKeyName, &subKeyNameSize,
										nullptr, nullptr, nullptr, nullptr);

			if (result == ERROR_NO_MORE_ITEMS)
				break;

			if (result == ERROR_SUCCESS)
			{
				std::string newSubKey = subKey + "\\" + subKeyName;
				cleanRegistry(hKey, newSubKey);
			}
			index++;
		}

		bool isError = deleteKey(key, subKey);
		RegCloseKey(key);
		if (isError)
		{
			logger->log("Error creating the backup!");
			return 1;
		}
		return 0;
	}

	void restoreBackup()
	{
		std::ifstream backup(logger->backupPath);
		if (!backup.is_open())
		{
			logger->log("Failed to open backup file: " + logger->backupPath);
			return;
		}

		std::string line;
		size_t restored_count = 0;
		size_t failed_count = 0;

		while (std::getline(backup, line))
		{
			size_t delimPos = line.find('=');
			if (delimPos == std::string::npos)
			{
				logger->log("Invalid backup line format: " + line);
				failed_count++;
				continue;
			}

			std::string fullPath = line.substr(0, delimPos);
			std::string valueData = line.substr(delimPos + 1);

			size_t lastSlash = fullPath.find_last_of('\\');
			if (lastSlash == std::string::npos)
			{
				logger->log("Invalid path format: " + line);
				failed_count++;
				continue;
			}

			std::string subKey = fullPath.substr(0, lastSlash);
			std::string valueName = fullPath.substr(lastSlash + 1);

			HKEY hKey;
			LONG result = RegCreateKeyExA(HKEY_CURRENT_USER, subKey.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);

			if (result == ERROR_SUCCESS)
			{
				result = RegSetValueExA(
					hKey,
					valueName.c_str(),
					0,
					REG_SZ,
					(const BYTE *)valueData.c_str(),
					static_cast<DWORD>(valueData.length() + 1));

				if (result == ERROR_SUCCESS)
				{
					logger->log("Value restored: " + fullPath + " -> " + valueData);
					restored_count++;
				}
				else
				{
					logger->log("Failed to restore value: " + fullPath +
								" (Error code: " + std::to_string(result) + ")");
					failed_count++;
				}
				RegCloseKey(hKey);
			}
			else
			{
				logger->log("Failed to create/open key for restore: " + subKey +
							" (Error code: " + std::to_string(result) + ")");
				failed_count++;
			}
		}
		backup.close();
		if (restored_count > 0 || failed_count > 0)
		{
			logger->log("Backup restoration completed:");
			logger->log("Successfully restored: " + std::to_string(restored_count) + " keys");
			if (failed_count > 0)
				logger->log("Failed to restore: " + std::to_string(failed_count) + " keys");
		}
		else
			logger->log("Backup file is empty or contains no valid entries!");
	}
};