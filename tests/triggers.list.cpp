#include <gtest/gtest.h>

#include "engine/scripting/dg_scripts.h"
#include "engine/db/db.h"

class TriggersList_F : public ::testing::Test
{
protected:
	using test_triggers_list_t = std::list<Trigger*>;

	constexpr static int TRIGGERS_NUMBER = 12;

	virtual void SetUp() override;

	void populate_tests_triggers_list();
	void remove_in_nested_loop();

	class TrigIndexRemover	// this class needed to make sure that global trig_index will be deleted last
	{
	public:
		~TrigIndexRemover()
		{
			for (int i = 0; i < top_of_trigt; ++i)
			{
				delete trig_index[i]->proto;

				free(trig_index[i]);
			}

			delete[] trig_index;
		}
	} m_trig_index_remover;
	Script m_script;
	test_triggers_list_t m_test_triggers;	// so far triggers are being deleted when they are being removed from TriggersList. So, we shouldn't care about that.
};

const int TriggersList_F::TRIGGERS_NUMBER;

void TriggersList_F::SetUp()
{
	trigger_list = GlobalTriggersStorage(); // reset global triggers storage before each test
	trig_index = new IndexData*[TRIGGERS_NUMBER];
	top_of_trigt = 0;

	for (int i = 0; i < TRIGGERS_NUMBER; ++i)
	{
		const auto trigger = new Trigger();

		trigger->set_rnum(i);

		AddTrigIndexEntry(i, trigger);
	}
}

void TriggersList_F::populate_tests_triggers_list()
{
	for (int i = 0; i < top_of_trigt; ++i)
	{
		Trigger* trigger = read_trigger(i);
		m_test_triggers.push_back(trigger);

		EXPECT_NO_THROW(add_trigger(&m_script, trigger, -1));
	}
}

TEST_F(TriggersList_F, AddBack)
{
	for (test_triggers_list_t::const_iterator i = m_test_triggers.begin(); i != m_test_triggers.end(); ++i)
	{
		Trigger* trigger = read_trigger((*i)->get_rnum());

		EXPECT_NO_THROW(add_trigger(&m_script, trigger, -1));
	}

	test_triggers_list_t::const_iterator i = m_test_triggers.begin();
	for (auto t : m_script.script_trig_list)
	{
		ASSERT_NE(m_test_triggers.end(), i);
		EXPECT_EQ((*i)->get_rnum(), t->get_rnum());
		++i;
	}
}

TEST_F(TriggersList_F, AddFront)
{
	for (test_triggers_list_t::const_iterator i = m_test_triggers.begin(); i != m_test_triggers.end(); ++i)
	{
		Trigger* trigger = read_trigger((*i)->get_rnum());

		EXPECT_NO_THROW(add_trigger(&m_script, trigger, 0));
	}

	test_triggers_list_t::const_reverse_iterator i = m_test_triggers.rbegin();
	for (auto t : m_script.script_trig_list)
	{
		ASSERT_NE(m_test_triggers.rend(), i);
		EXPECT_EQ((*i)->get_rnum(), t->get_rnum());
		++i;
	}
}

TEST_F(TriggersList_F, RemoveOneWhenIterating)
{
	populate_tests_triggers_list();

	int counter = 0;
	for (auto t : m_script.script_trig_list)
	{
		++counter;
		m_script.script_trig_list.remove(t);
	}

	EXPECT_EQ(counter, TRIGGERS_NUMBER);
	EXPECT_TRUE(m_script.script_trig_list.empty());
}

TEST_F(TriggersList_F, RemoveAllWhenIterating)
{
	populate_tests_triggers_list();

	int counter = 0;
	for (auto t : m_script.script_trig_list)
	{
		UNUSED_ARG(t);
		++counter;
		for (auto to_remove : m_test_triggers)
		{
			m_script.script_trig_list.remove(to_remove);
		}
	}

	EXPECT_EQ(1, counter);
	EXPECT_TRUE(m_script.script_trig_list.empty());
}

TEST_F(TriggersList_F, NestedLoops)
{
	populate_tests_triggers_list();

	int inner_counter = 0;
	int outer_counter = 0;
	for (auto to : m_script.script_trig_list)
	{
		++outer_counter;
		UNUSED_ARG(to);
		for (auto ti : m_script.script_trig_list)
		{
			UNUSED_ARG(ti);
			++inner_counter;
		}
	}

	EXPECT_EQ(outer_counter, TRIGGERS_NUMBER);
	EXPECT_EQ(inner_counter, TRIGGERS_NUMBER*TRIGGERS_NUMBER);
}

void TriggersList_F::remove_in_nested_loop()
{
	int inner_counter = 0;
	int outer_counter = 0;
	for (auto t : m_script.script_trig_list)
	{
		++outer_counter;
		UNUSED_ARG(t);
		for (auto t : m_script.script_trig_list)
		{
			++inner_counter;
			if (0 == inner_counter % 2)
			{
				m_script.script_trig_list.remove(t);
			}
		}
	}

	exit(0);
}

TEST_F(TriggersList_F, NestedLoops_WithRemoving)
{
	populate_tests_triggers_list();

	EXPECT_EXIT(remove_in_nested_loop(), ::testing::ExitedWithCode(0), ".*");
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
