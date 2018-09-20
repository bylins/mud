// by bodrich (2014)
// http://mud.ru

class Quest
{
    public:
	Quest(int id, int time_start, int time_end, std::string text_quest, std::string tquest, int var_quest);
	//~Quest();
	int get_id();
	int get_time_start();
	int get_time_end();
	int get_time();
	std::string get_text();
	std::string get_tquest();
	int get_var_quest();
	int pquest();
	void set_pvar(int pvar);
    private:
	// id квеста
	int id;
	// время, когда был взят квест
	int time_start;
	// время, когда нужно сдать квест
	int time_end;
	// описание квеста
	std::string text_quest;
	// краткое описание
	std::string tquest;
	// переменная для квеста, сколько надо убить мобов, принести айтемов и тд
	int var_quest;
	// сколько уже выполнено
	int pvar_quest;
};

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
