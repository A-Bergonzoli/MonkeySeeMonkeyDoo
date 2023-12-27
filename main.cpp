#include <algorithm>
#include <cstdio>
#include <curses.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <unistd.h>
#include <vector>

constexpr uint8_t NORMAL = 0;
constexpr uint8_t HIGHLIGTHED = 1;
constexpr uint8_t ID_TODO = 0;
constexpr uint8_t ID_DONE = 1;

// @TODO [OPT] Print information on creation / completion of task

class ListManager;
class UI;

using CallbackSingleFocus = std::function<void(ListManager&)>;
using CallbackBothFocus = std::function<void(ListManager&, ListManager&)>;
using CallbackWithUI = std::function<void(UI&, ListManager&)>;

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
    Cursor(int x = 0, int y = 0)
        : c_x_(x)
        , c_y_(y)
    {
    }

    void up()
    {
        if (c_x_ > 0)
            decreaseRow(1);
    }

    void down(std::size_t max_len)
    {
        if (c_x_ <= max_len)
            increaseRow(1);
    }

    void left()
    {
        if (c_y_ > 0)
            decreaseCol(1);
    }

    void right(std::size_t max_len)
    {
        if (c_y_ <= max_len)
            increaseCol(1);
    }

    void decreaseCol(uint8_t amount) { c_y_ -= amount; }

    void increaseCol(uint8_t amount) { c_y_ += amount; }

    void decreaseRow(uint8_t amount) { c_x_ -= amount; }

    void increaseRow(uint8_t amount) { c_x_ += amount; }

    void resetCol() { c_y_ = 0; }

    void resetRow() { c_x_ = 0; }

    int col() const { return c_y_; }

    int row() const { return c_x_; }

private:
    int c_x_;
    int c_y_;
};

class Window {
    // @TODO: implement
    // scroll up/down wrapping around screen
};

enum class Focus {
    TODO,
    DONE,
    NEW_TASK
};

class ListManager {
    using List = std::vector<std::string>;

public:
    ListManager(const std::string& file, const std::string& prefix)
        : file_name_(file)
        , prefix_(prefix)
    {
        ReadList(file_name_);
    }

    std::string remove(int pos)
    {
        std::string erased_value = list_.at(pos);
        const auto it = list_.erase(std::cbegin(list_) + pos);

        return erased_value;
    }

    void emplace_back(std::string task)
    {
        list_.emplace_back(task);
    }

    List GetList() const { return list_; }

    void move_element(size_t to, size_t from, const std::string& task)
    {
        list_.erase(std::begin(list_) + from);
        list_.insert(std::begin(list_) + to, task);
    }

    void ReadList(std::string from_location)
    {
        std::fstream in_file(from_location, std::ios::in);
        std::string line;

        if (in_file.is_open()) {
            while (std::getline(in_file, line)) {
                if (line.rfind(prefix_, 0) == 0)
                    list_.emplace_back(line.substr(prefix_.length()));
            }
        }

        in_file.close();
    }

    void SaveList(std::string to_location)
    {
        std::fstream out_file(to_location, std::ofstream::app);

        if (out_file.is_open()) {
            for (const auto& task : list_)
                out_file << prefix_ + task << "\n";
        }

        out_file.close();
    }

    void AddCurrent(int n) { current_ += n; }

    void ResetCurrent() { current_ = 0; }

    void SetCurrent(int n) { current_ = n; }

    int GetCurrent() const { return current_; }

private:
    List list_;
    int current_ { 0 };
    std::string prefix_ {};
    std::string file_name_ {};
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

    void list_new_element(const std::string& entry)
    {
        curs_set(1);
        move(1, 0);
        clrtoeol();
        attron(COLOR_PAIR(NORMAL));
        mvaddstr(1, 0, entry.c_str());
        attroff(COLOR_PAIR(NORMAL));
    }

    void SwitchFocusTo(Focus focus)
    {
        focus_ = focus;
    }

    void ResetDrawPosition()
    {
        pos_draw->Update(-42, -42);
    }

    void ToggleFocusDoneTodo()
    {
        focus_ = (focus_ == Focus::TODO) ? Focus::DONE : Focus::TODO;
    }

    Focus ReturnFocus() const { return focus_; }

private:
    std::unique_ptr<Position> pos_draw;
    std::optional<Id> current_list_;
    bool focus_todo_ { true };
    Focus focus_ { Focus::TODO };
};

class User {
public:
    explicit User(CallbackSingleFocus cb)
        : callback_(std::move(cb))
    {
    }

    void invoke(ListManager& lm)
    {
        callback_(lm);
    }

private:
    CallbackSingleFocus callback_;
};

class DemandingUser {
public:
    explicit DemandingUser(CallbackBothFocus cb)
        : callback_(std::move(cb))
    {
    }

    void invoke(ListManager& todo_lm, ListManager& done_lm)
    {
        callback_(todo_lm, done_lm);
    }

private:
    CallbackBothFocus callback_;
};

class UIUser {
public:
    explicit UIUser(CallbackWithUI cb)
        : callback_(std::move(cb))
    {
    }

    void invoke(UI& ui, ListManager& done_lm)
    {
        callback_(ui, done_lm);
    }

private:
    CallbackWithUI callback_;
};

struct MoveUp {
    void operator()(ListManager& lm)
    {
        lm.AddCurrent(-1);
        if (lm.GetCurrent() < 0)
            lm.SetCurrent(lm.GetList().size() - 1);
    }
};
struct MoveDown {
    void operator()(ListManager& lm)
    {
        lm.AddCurrent(1);
        if (lm.GetCurrent() > lm.GetList().size() - 1)
            lm.SetCurrent(0);
    }
};
struct MarkDoneTodo {
    void operator()(ListManager& todo_lm, ListManager& done_lm)
    {
        if (todo_lm.GetList().empty())
            return;

        const auto task = todo_lm.remove(todo_lm.GetCurrent());
        done_lm.emplace_back(task);
        if (todo_lm.GetCurrent() - 1 >= 0)
            todo_lm.AddCurrent(-1);
    }
};
struct MovePriorityUp {
    void operator()(ListManager& lm)
    {
        if (lm.GetCurrent() - 1 < 0)
            return;
        lm.move_element(lm.GetCurrent() - 1, lm.GetCurrent(), lm.GetList().at(lm.GetCurrent()));
        lm.AddCurrent(-1);
    }
};
struct MovePriorityDown {
    void operator()(ListManager& lm)
    {
        if (lm.GetCurrent() + 1 > lm.GetList().size() - 1)
            return;
        lm.move_element(lm.GetCurrent() + 1, lm.GetCurrent(), lm.GetList().at(lm.GetCurrent()));
        lm.AddCurrent(1);
    }
};
struct AppendNewTask {
    void operator()(UI& ui, ListManager& todo_lm)
    {
        ui.SwitchFocusTo(Focus::NEW_TASK);
        ui.label("NEW TASK");
        std::string buf {};
        char ch = getch();

        while (ch != (char)10) // ESC
        {
            switch (ch) {
            case (char)KEY_BACKSPACE: {
                if (!buf.empty())
                    buf.erase(cursor_.col() - 1, 1);
                cursor_.left();
                break;
            }
            case (char)KEY_LEFT: {
                cursor_.left();
                break;
            }
            case (char)KEY_RIGHT: {
                cursor_.right(buf.length());
                break;
            }
            default:
                buf.insert(cursor_.col(), std::string(1, ch));
                cursor_.right(buf.length());
                break;
            }
            ui.list_new_element(buf);
            move(0, 0);
            move(cursor_.row(), cursor_.col());
            refresh();
            ch = getch();
        }

        todo_lm.ResetCurrent();
        todo_lm.emplace_back(buf);
        refresh();
        ui.SwitchFocusTo(Focus::TODO);
        ui.ResetDrawPosition();
    }

    Cursor cursor_ { 1, 0 };
};
struct DeleteTask {
    void operator()(ListManager& lm)
    {
        if (lm.GetList().empty())
            return;

        lm.remove(lm.GetCurrent());
    }
};

std::string GetRandomName(int len)
{
    const std::string extension = ".todo";
    static const char alphanum[] = "0123456789"
                                   "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                   "abcdefghijklmnopqrstuvwxyz";
    std::string rand_s = "new_";

    for (int i = 0; i < len; ++i)
        rand_s += alphanum[rand() % (sizeof(alphanum) - 1)];

    return rand_s + extension;
}

int main(int argc, char* argv[])
{
    srand((unsigned)time(NULL) * getpid());
    bool quit = false;
    std::string file_path {};

    if (argv[1])
        file_path = argv[1];
    else
        file_path = GetRandomName(5);

    // init cursors w/ desidered config
    initscr();
    noecho();
    keypad(stdscr, true);

    // use colors :)
    start_color();
    init_pair(NORMAL, COLOR_WHITE, COLOR_BLACK);
    init_pair(HIGHLIGTHED, COLOR_BLACK, COLOR_WHITE);

    UI ui { std::make_unique<Position>() };
    ListManager todos_manager { file_path, "TODO " };
    ListManager dones_manager { file_path, "DONE " };

    while (!quit) {
        clear();
        curs_set(0);

        if (ui.ReturnFocus() == Focus::TODO) {
            ui.label("[TODO] DONE ");
            ui.begin_list(ID_TODO);
            for (auto i = 0; i < todos_manager.GetList().size(); ++i)
                ui.list_element(todos_manager.GetList().at(i), i, todos_manager.GetCurrent());
            ui.end_list();
        } else {
            ui.label(" TODO [DONE]");
            ui.begin_list(ID_DONE);
            for (auto i = 0; i < dones_manager.GetList().size(); ++i)
                ui.list_element(dones_manager.GetList().at(i), i, dones_manager.GetCurrent());
            ui.end_list();
        }

        ui.ResetDrawPosition();

        refresh();
        char ch = getch();

        switch (ch) {
        case 'q': {
            if (std::ifstream(file_path))
                std::remove(file_path.c_str());
            todos_manager.SaveList(file_path);
            dones_manager.SaveList(file_path);
            quit = true;
            break;
        }
        case (char)KEY_UP: {
            User user(MoveUp {});
            if (ui.ReturnFocus() == Focus::TODO) {
                user.invoke(todos_manager);
            } else {
                user.invoke(dones_manager);
            }
            break;
        }
        case (char)KEY_DOWN: {
            User user(MoveDown {});
            if (ui.ReturnFocus() == Focus::TODO) {
                user.invoke(todos_manager);
            } else {
                user.invoke(dones_manager);
            }
            break;
        }
        case (char)'\t': {
            ui.ToggleFocusDoneTodo();
            break;
        }
        case (char)10: {
            DemandingUser user(MarkDoneTodo {});
            if (ui.ReturnFocus() == Focus::TODO) {
                user.invoke(todos_manager, dones_manager);
            }

            if (ui.ReturnFocus() == Focus::DONE) {
                user.invoke(dones_manager, todos_manager);
            }

            ui.ResetDrawPosition();
            break;
        }
        case 'Q': { // shifted up
            User user(MovePriorityUp {});
            if (ui.ReturnFocus() == Focus::TODO) {
                user.invoke(todos_manager);
            }

            if (ui.ReturnFocus() == Focus::DONE) {
                user.invoke(dones_manager);
            }

            break;
        }
        case 'P': { // shifted down
            User user(MovePriorityDown {});
            if (ui.ReturnFocus() == Focus::TODO) {
                user.invoke(todos_manager);
            }

            if (ui.ReturnFocus() == Focus::DONE) {
                user.invoke(dones_manager);
            }

            break;
        }
        case 'i': {
            UIUser user(AppendNewTask {});
            if (ui.ReturnFocus() == Focus::TODO) {
                clear();
                user.invoke(ui, todos_manager);
            }

            break;
        }
        case 'x': {
            User user(DeleteTask {});
            if (ui.ReturnFocus() == Focus::DONE) {
                user.invoke(dones_manager);
            }

            break;
        }
        default:
            break;
        }
    }
    endwin();

    return 0;
}
