#!/bin/bash

SAVE_FILE="tic_tac_toe_board.txt"

init_board() {
    board=(" " " " " " " " " " " " " " " " " ")
}

print_board() {
    clear
    echo "Текущая игровая доска:"
    echo " ${board[0]} | ${board[1]} | ${board[2]} "
    echo "---+---+---"
    echo " ${board[3]} | ${board[4]} | ${board[5]} "
    echo "---+---+---"
    echo " ${board[6]} | ${board[7]} | ${board[8]} "
    echo
    echo "Текущая игровая доска:" >$SAVE_FILE
    echo " ${board[0]} | ${board[1]} | ${board[2]} " >>$SAVE_FILE
    echo "---+---+---" >>$SAVE_FILE
    echo " ${board[3]} | ${board[4]} | ${board[5]} " >>$SAVE_FILE
    echo "---+---+---" >>$SAVE_FILE
    echo " ${board[6]} | ${board[7]} | ${board[8]} " >>$SAVE_FILE
}

check_winner() {
    WIN_COMBINATIONS=(
        "0 1 2" "3 4 5" "6 7 8"
        "0 3 6" "1 4 7" "2 5 8"
        "0 4 8" "2 4 6"
    )

    for combo in "${WIN_COMBINATIONS[@]}"; do
        set -- $combo
        if [[ "${board[$1]}" != " " && "${board[$1]}" == "${board[$2]}" && "${board[$2]}" == "${board[$3]}" ]]; then
            echo "Игрок ${board[$1]} победил!"
            return 0
        fi
    done

    return 1
}

check_draw() {
    for cell in "${board[@]}"; do
        if [[ "$cell" == " " ]]; then
            return 1
        fi
    done
    echo "Ничья!"
    return 2
}

# Ход игрока
player_move() {
    local player=$1
    print_board
    while true; do
        echo "Ход игрока $player. Введите позицию (1-9):"
        read -r pos
        if [[ -z "$pos" || ! "$pos" =~ ^[1-9]$ || "${board[$((pos - 1))]}" != " " ]]; then
            echo "Некорректный ввод. Попробуйте снова."
            continue
        fi
        board[$((pos - 1))]=$player # Обновляем игровое поле
        break                       # Выходим из цикла после успешного хода
    done
}

play_game() {
    init_board
    print_board
    local current_player="X"
    while true; do
        player_move $current_player
        print_board
        check_winner
        result=$?
        if [[ $result -eq 0 ]]; then
            echo "Поздравляем, победил игрок $current_player!"
            break
        fi

        check_draw
        result=$?
        if [[ $result -eq 2 ]]; then
            echo "Игра завершилась ничьей!"
            break
        fi

        current_player=$([[ $current_player == "X" ]] && echo "O" || echo "X")
    done
    echo
    echo "Игра окончена. Спасибо за игру!"
}

play_game
