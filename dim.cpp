#include <ncurses.h>
#include <string>
#include <stdexcept>
#include <cmath>
#include <fstream>

#include "dim.h"

int main(int argc, char* argv[]) {
    Dim* editor = new Dim();
    if (argc == 2) {
        editor->filename = argv[1];
    } else {
        editor->filename = "";
    }

    editor->run();
    delete editor;

    return 0;
}

Dim::Dim() : cursor_y(0), cursor_x(0) {
    current_mode = EditorMode::COMMAND;
    indent = 4;

    mainwin = initscr();
    cbreak();
    noecho();
    keypad(mainwin, TRUE);

    buffer.push_back("");
}

Dim::~Dim() {
    endwin();
}

void Dim::run() {
    while (!quit) {
        display();
        int ch = getch();
        handle_input(ch);
    }

    clear();
}

void Dim::display() {
    clear();
    for (size_t i = 0; i < buffer.size(); ++i) {
        mvaddstr(i, 0, buffer[i].c_str());
    }

    int rows, cols;
    getmaxyx(mainwin, rows, cols);
    std::string mode_str = mode_to_string(current_mode);
    attron(A_REVERSE);
    mvprintw(rows - 1, 0, "%-*s", cols, mode_str.c_str());
    attroff(A_REVERSE);

    move(cursor_y, cursor_x);

    refresh();
}

void Dim::handle_input(int ch) {
    switch (current_mode) {
        case EditorMode::COMMAND:
            handle_command(ch);
            break;
        case EditorMode::INSERT:
            handle_insert(ch);
            break;
        case EditorMode::VISUAL:
            //handle_visual(ch);
            break;
        default:
            throw std::logic_error("Unknown editor mode in handle_input");
    }
}

void Dim::write_to_file() {
    if (filename != "") {
        std::ofstream output_file(filename);

        for (size_t i = 0; i < buffer.size(); i++) {
            output_file << buffer[i] << (i == buffer.size()-1 ? "" : "\n");
        }

        output_file.close();
    }
}

void Dim::handle_multistroke(int ch) {
    char start_char = static_cast<char>(ch);
    command_buffer.clear();
    mvwaddch(stdscr, LINES - 1, 0, start_char);
    int ch_command;

    if (start_char == ':') {
        while ((ch_command = getch()) != ERR) {
            if (ch_command == KEY_ENTER || ch_command == '\n') {
                break;
            }
            else if (ch_command == 27) {    // ESC
                command_buffer.clear();
                break;
            }
            command_buffer += static_cast<char>(ch_command);
            mvwprintw(mainwin, LINES - 1, command_buffer.length(), "%c", ch_command);
        }
    }

    /* 
        Write commands 
    */
    if (command_buffer == "w") {
        write_to_file();
    } 
    else if (command_buffer == "wq") {
        write_to_file();
        quit = true;
    }
    /*
        Search
    */
    else if (start_char == '/') {
        // Do nothing (yet)
    }

    command_buffer.clear();
}

void Dim::handle_command(int ch) {
    // Multi-stroke commands, just a few for now:
    if (ch == ':' || ch == '/' || ch == 'd') {
        handle_multistroke(ch);
    } else {
        // Core stuff
        switch (ch) {
            case static_cast<int>('i'):
                current_mode = EditorMode::INSERT;
                alignment = AlignmentMode::UNALIGNED;
                break;
            case static_cast<int>('a'):
                current_mode = EditorMode::INSERT;
                alignment = AlignmentMode::UNALIGNED;

                // Go forward a character or add to the end of the line if not present
                if (buffer[cursor_y].length() > cursor_x) {
                    ++cursor_x;
                } else {
                    buffer[cursor_y].push_back(' ');
                    ++cursor_x;
                }

                break;
            case static_cast<int>('j'):
                if ((cursor_y < static_cast<unsigned int>(buffer.size() - 1))) {
                    ++cursor_y;
                    cursor_x = find_line_pos(cursor_x, buffer[cursor_y].size());
                }
                break;
            case static_cast<int>('k'):
                if (cursor_y > 0) {
                    --cursor_y;
                    cursor_x = find_line_pos(cursor_x, buffer[cursor_y].size());
                }
                break;
            case static_cast<int>('l'):
                alignment = AlignmentMode::UNALIGNED;
                if (cursor_x < static_cast<unsigned int>(buffer[cursor_y].length())) {
                    ++cursor_x;
                }
                break;
            case static_cast<int>('h'):
                alignment = AlignmentMode::UNALIGNED;
                if (cursor_x > 0) {
                    --cursor_x;
                }
                break;
            case static_cast<int>('x'):
                alignment = AlignmentMode::UNALIGNED;
                if (buffer[cursor_y].length() > 0) {
                    buffer[cursor_y].erase(cursor_x, 1);
                }

                break;
            case static_cast<int>('$'):
                cursor_x = buffer[cursor_y].length()-1;
                alignment = AlignmentMode::RIGHT;
                break;
            case static_cast<int>('0'):
                cursor_x = 0;
                alignment = AlignmentMode::LEFT;
                break;
            case static_cast<int>('w'):
                {
                    alignment = AlignmentMode::UNALIGNED;
                    int idx = find_next_w(buffer[cursor_y], cursor_x);
                    if (idx != -1) {
                        cursor_x = idx;
                    }
                }
                break;
            case static_cast<int>('o'):
                current_mode = EditorMode::INSERT;
                alignment = AlignmentMode::UNALIGNED;
                {
                    cursor_x = indentation(buffer[cursor_y]);
                    std::string s = "";
                    for (size_t i = 0; i < cursor_x; i++) {
                        s += ' ';
                    }

                    if (cursor_y == static_cast<unsigned int>(buffer.size() - 1)) {
                        buffer.push_back(s);
                        ++cursor_y;
                    }
                    else if (cursor_y < static_cast<unsigned int>(buffer.size() - 1)) {
                        buffer.insert(buffer.begin() + cursor_y + 1, s);
                        ++cursor_y;
                    }
                }

                break;
            case static_cast<int>('O'):
                current_mode = EditorMode::INSERT;
                alignment = AlignmentMode::UNALIGNED;
                {
                    cursor_x = indentation(buffer[cursor_y]);
                    std::string s = "";
                    for (size_t i = 0; i < cursor_x; i++) {
                        s += ' ';
                    }

                    buffer.insert(buffer.begin() + cursor_y, s);
                }
                break;
        }
    }
}

void Dim::handle_insert(int ch) {
    switch (ch) {
        case 27:    // ESC key
            current_mode = EditorMode::COMMAND;
            if (cursor_x > 0) {
                --cursor_x;
            }
            break;
        case KEY_ENTER:
        case '\n':
        case '\r':
            {
                cursor_x = indentation(buffer[cursor_y]);
                std::string s = "";
                for (size_t i = 0; i < cursor_x; i++) {
                    s += ' ';
                }

                if (cursor_y == static_cast<unsigned int>(buffer.size() - 1)) {
                    buffer.push_back(s);
                    ++cursor_y;
                }
                else if (cursor_y < static_cast<unsigned int>(buffer.size()-1)) {
                    buffer.insert(buffer.begin() + cursor_y + 1, s);
                    ++cursor_y;
                }
            }
            break;
        case KEY_BACKSPACE:
            if (cursor_x > 0) {
                if (cursor_x > 0 && buffer[cursor_y][cursor_x - 1] == ' ') {
                    unsigned int spaces_to_remove = cursor_x % indent;
                    if (spaces_to_remove == 0) {
                        spaces_to_remove = indent;
                    } else if (spaces_to_remove > cursor_x) {
                        spaces_to_remove = cursor_x;
                    }
                    
                    buffer[cursor_y].erase(cursor_x - spaces_to_remove, spaces_to_remove);
                    cursor_x -= spaces_to_remove;
                } else {
                    buffer[cursor_y].erase(cursor_x - 1, 1);
                    --cursor_x;
                }
            } else if (cursor_y > 0) {
                unsigned int prev_line_length = buffer[cursor_y - 1].length();
                buffer[cursor_y - 1] += buffer[cursor_y];
                buffer.erase(buffer.begin() + cursor_y);
                --cursor_y;
                cursor_x = prev_line_length;
            }
            break;
        case '\t':
        case KEY_BTAB:
            {
                std::string s = "";
                for (size_t i = 0; i < indent; i++) {
                    s += ' ';
                }
                buffer[cursor_y].insert(cursor_x, s);
                cursor_x += indent;
            }

            break;
        default:
            if (ch >= 32 && ch <= 126) {
                buffer[cursor_y].insert(cursor_x, 1, static_cast<char>(ch));
                ++cursor_x;
            }
            break;
    }
}

// void Dim::handle_visual(int ch) {

// }

int Dim::find_line_pos(int current_x, int next_length) {
    if (alignment == AlignmentMode::LEFT) {
        return 0;
    }
    
    if (alignment == AlignmentMode::RIGHT) {
        return next_length;
    }

    return (current_x <= next_length ? current_x : next_length);
}

int Dim::indentation(std::string line) {
    for (size_t i = 0; i < line.size(); i++) {
        if (line[i] != ' ') {
            return i;
        }
    }

    return 0;
}

int Dim::find_next_w(std::string line, int current_x) {
    for (size_t i = current_x+1; i < line.length(); i++) {
        if (line[i-1] == ' ' && isalnum(line[i])) {
            return i;
        } else if ((line[i] >= 33 && line[i] <= 47) || (line[i] >= 58 && line[i] <= 63)) {
            return i;
        }
    }

    return -1;
}

std::string Dim::mode_to_string(EditorMode mode) {
    switch (mode) {
        case EditorMode::COMMAND:
            return "COMMAND";
        case EditorMode::INSERT:
            return "INSERT";
        case EditorMode::VISUAL:
            return "VISUAL";
        default:
            throw std::logic_error("Unknown mode state in mode_to_string");
    }
}