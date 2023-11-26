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

// @TODO [OPT] Delete tasks from DONE by pressing 'x'
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
    Cursor(std::unique_ptr<Position> pos_user)
        : pos_user_(std::move(pos_user))
    {
    }

    void Update(int x, int y) { pos_user_->Update(x, y); }

private:
    std::unique_ptr<Position> pos_user_;
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

    void AddCurrent(int n) { current_ += n; }
    void SetCurrent(int n) { current_ = n; }
    int GetCurrent() const { return current_; }

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

    void list_new(const std::string& entry)
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
            if (ch == (char)KEY_BACKSPACE) {
                if (!buf.empty())
                    buf.pop_back();
            } else {
                buf += std::string(1, ch);
            }
            ui.list_new(buf);
            ch = getch();
        }

        todo_lm.AddCurrent(1);
        todo_lm.emplace_back(buf);
        refresh();
        ui.SwitchFocusTo(Focus::TODO);
        ui.ResetDrawPosition();
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
    Cursor cursor { std::make_unique<Position>() };
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
            cursor.Update(-1, 0);
            break;
        }
        case (char)KEY_DOWN: {
            User user(MoveDown {});
            if (ui.ReturnFocus() == Focus::TODO) {
                user.invoke(todos_manager);
            } else {
                user.invoke(dones_manager);
            }
            cursor.Update(1, 0);
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

            cursor.Update(-1, 0);
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

            cursor.Update(-1, 0);
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

            cursor.Update(1, 0);
            break;
        }
        case 'i': {
            UIUser user(AppendNewTask {});
            if (ui.ReturnFocus() == Focus::TODO) {
                clear();
                user.invoke(ui, todos_manager);
            }

            cursor.Update(1, 0);
            break;
        }
        default:
            break;
        }
    }
    endwin();

    return 0;
}
