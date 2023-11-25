// #include <cstdint>
#include <cstddef>
#include <cstdio>
#include <curses.h>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

constexpr uint8_t NORMAL = 0;
constexpr uint8_t HIGHLIGTHED = 1;
constexpr uint8_t ID_TODO = 0;
constexpr uint8_t ID_DONE = 1;

// @TODO Make items go from TODO to Done and viceversa
// @TODO Read/Write from File
// @TODO [OPT] Move task up/down (give priority) by pressing SHIFT+KEY_UP/DOWN
// @TODO [OPT] Print information on creation / completion of task

struct Position {
    int row = 0;
    int col = 0;

    void Update(int mv_x, int mv_y)
    {
        row = (row + mv_x < 0) ? 0 : row + mv_x;
        col = (col + mv_y < 0) ? 0 : col + mv_y;
    }
};

class Cursor {
public:
    Cursor(std::unique_ptr<Position> pos_user)
        : pos_user_(std::move(pos_user))
    {
    }

    void Update(int x, int y) { pos_user_->Update(x, y); }

private:
    std::unique_ptr<Position> pos_user_;
};

class Window {
    // @TODO: imeplemnt
    // ha un cursor
    // serve per scollare wrappando
};

struct ItemList {
    size_t id;
    bool focus { false };
};

class UI {
public:
    using Id = size_t;

    UI(std::unique_ptr<Position> pos_drawptr)
        : pos_draw(std::move(pos_drawptr))
    {
    }

    void label(const std::string& label)
    {
        attron(COLOR_PAIR(NORMAL));
        mvaddstr(pos_draw->row, pos_draw->col, label.c_str());
        attroff(COLOR_PAIR(NORMAL));

        pos_draw->Update(1, 0);
    }

    void begin_list(Id id)
    {
        if (current_list_.has_value())
            std::cerr << "Error: nested list are not supported.\n";
        current_list_ = id;
    }

    void end_list()
    {
        current_list_ = std::nullopt;
    }

    void list_element(const std::string& item, int index, int current)
    {
        const uint8_t pair = (index == current) ? HIGHLIGTHED : NORMAL;
        const std::string prefix = (current_list_ == ID_TODO) ? "- [ ] " : "- [x] ";
        attron(COLOR_PAIR(pair));
        mvaddstr(pos_draw->row, 0, (prefix + item).c_str());
        attroff(COLOR_PAIR(pair));

        pos_draw->Update(1, 0);
    }

    void ResetDrawPosition()
    {
        pos_draw->Update(-42, -42);
    }

    void SwitchFocus()
    {
        focus_todo_ = (focus_todo_) ? false : true;
    }

    bool IsFocusTodo() const { return focus_todo_; }

private:
    std::unique_ptr<Position> pos_draw;
    std::optional<Id> current_list_;
    bool focus_todo_ { true };
};

int main()
{
    bool quit = false;

    // init cursors w/ desidered config
    initscr();
    noecho();
    keypad(stdscr, true);
    curs_set(0);

    // use colors :)
    start_color();
    init_pair(NORMAL, COLOR_WHITE, COLOR_BLACK);
    init_pair(HIGHLIGTHED, COLOR_BLACK, COLOR_WHITE);

    // coming from user
    std::vector<std::string> todos = {
        "Buy bread",
        "Do the groceries",
        "Take aspirin"
    };
    std::vector<std::string> dones = {
        "Run diswhasher",
        "Run laundry",
        "Buy water"
    };
    int todo_current { 0 };
    int done_current { 1 };

    UI ui { std::make_unique<Position>() };
    Cursor cursor { std::make_unique<Position>() };

    while (!quit) {
        clear();
        // ui.begin()

        if (ui.IsFocusTodo()) {
            ui.label("[TODO] DONE ");
            ui.begin_list(ID_TODO);
            for (auto i = 0; i < todos.size(); ++i)
                ui.list_element(todos.at(i), i, todo_current);
            ui.end_list();
        } else {
            ui.label(" TODO [DONE]");
            ui.begin_list(ID_DONE);
            for (auto i = 0; i < dones.size(); ++i)
                ui.list_element(dones.at(i), i, done_current);
            ui.end_list();
        }

        ui.ResetDrawPosition();
        // ui.end();

        refresh();
        char ch = getch();

        switch (ch) {
        case 'q':
            quit = true;
            break;
        case (char)KEY_UP: {
            if (--todo_current < 0)
                todo_current = todos.size() - 1;
            cursor.Update(-1, 0);
            break;
        }
        case (char)KEY_DOWN: {
            if (++todo_current > todos.size() - 1)
                todo_current = 0;
            cursor.Update(1, 0);
            break;
        }
        case (char)'\t': {
            ui.SwitchFocus();
        }
        default:
            break;
        }
    }
    endwin();

    return 0;
}
