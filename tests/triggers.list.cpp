#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <dg_scripts.h>
#include <db.h>

class Mock_TRIG_DATA : public TRIG_DATA
{
public:
	MOCK_METHOD0(destruct, void());

private:
	virtual ~Mock_TRIG_DATA() { destruct(); }
};

class TriggersList_F : public ::testing::Test
{
protected:
	using test_triggers_list_t = std::list<TRIG_DATA*>;

	constexpr static int TRIGGERS_NUMBER = 12;

	virtual void SetUp() override;

	void populate_tests_triggers_list();

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
	SCRIPT_DATA m_script;
	test_triggers_list_t m_test_triggers;	// so far triggers are being deleted when they are being removed from TriggersList. So, we shouldn't care about that.
};

const int TriggersList_F::TRIGGERS_NUMBER;

void TriggersList_F::SetUp()
{
	trigger_list = GlobalTriggersStorage(); // reset global triggers storage before each test
	trig_index = new INDEX_DATA*[TRIGGERS_NUMBER];
	top_of_trigt = 0;

	for (int i = 0; i < TRIGGERS_NUMBER; ++i)
	{
		const auto trigger = new Mock_TRIG_DATA();
		EXPECT_CALL(*trigger, destruct()).Times(1);

		trigger->set_rnum(i);

		add_trig_index_entry(i, trigger);
	}
}

void TriggersList_F::populate_tests_triggers_list()
{
	for (int i = 0; i < top_of_trigt; ++i)
	{
		TRIG_DATA* trigger = read_trigger(i);
		m_test_triggers.push_back(trigger);

		EXPECT_NO_THROW(add_trigger(&m_script, trigger, -1));
	}
}

TEST_F(TriggersList_F, AddBack)
{
	for (test_triggers_list_t::const_iterator i = m_test_triggers.begin(); i != m_test_triggers.end(); ++i)
	{
		TRIG_DATA* trigger = read_trigger((*i)->get_rnum());

		EXPECT_NO_THROW(add_trigger(&m_script, trigger, -1));
	}

	test_triggers_list_t::const_iterator i = m_test_triggers.begin();
	for (auto t : m_script.trig_list)
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
		TRIG_DATA* trigger = read_trigger((*i)->get_rnum());

		EXPECT_NO_THROW(add_trigger(&m_script, trigger, 0));
	}

	test_triggers_list_t::const_reverse_iterator i = m_test_triggers.rbegin();
	for (auto t : m_script.trig_list)
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
	for (auto t : m_script.trig_list)
	{
		++counter;
		m_script.trig_list.remove(t);
	}

	EXPECT_EQ(counter, TRIGGERS_NUMBER);
	EXPECT_TRUE(m_script.trig_list.empty());
}

TEST_F(TriggersList_F, RemoveAllWhenIterating)
{
	populate_tests_triggers_list();

	int counter = 0;
	for (auto t : m_script.trig_list)
	{
		UNUSED_ARG(t);
		++counter;
		for (auto to_remove : m_test_triggers)
		{
			m_script.trig_list.remove(to_remove);
		}
	}

	EXPECT_EQ(1, counter);
	EXPECT_TRUE(m_script.trig_list.empty());
}

TEST_F(TriggersList_F, NestedLoops)
{
	populate_tests_triggers_list();

	for (auto t : m_script.trig_list)
	{
		UNUSED_ARG(t);
		EXPECT_EQ(m_script.trig_list.begin(), m_script.trig_list.end());	// all nested loops must be locked
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
