#ifndef DIM_H
#define DIM_H

#include <ncurses.h>
#include <vector>
#include <string>

class Dim {
public:
    Dim();
    ~Dim();
    void run();
    void display();
    std::string filename;

    void write_to_file();

    void handle_command(int ch);
    void handle_multistroke(int ch);
    void handle_insert(int ch);
    //void handle_visual(int ch);
    void handle_input(int ch);

    int find_line_pos(int current_x, int next_length);
    static int indentation(std::string line);
    static int find_next_w(std::string line, int current_x);

    enum class EditorMode {
        INSERT,
        VISUAL,
        COMMAND
    };

    enum class AlignmentMode {
        LEFT,
        RIGHT,
        UNALIGNED
    };

    static std::string mode_to_string(EditorMode);

private:
    WINDOW* mainwin;
    std::vector<std::string> buffer;    /* The 'main' text buffer */
    unsigned int cursor_y, cursor_x;    /* Cursor Y, X coords on main buffer */
    unsigned int indent;    /* Number of spaces defined for indentation */
    std::string command_buffer;    /* Buffer for multiple-stroke commands */
    AlignmentMode alignment;  /* Keep track of line_end (0) and line_begin ($) presses for navigation. */
    EditorMode current_mode;    /* Current editor mode */
    bool quit = false;
};

#endif