#ifndef __MSDP_REPORTERS_HPP__
#define __MSDP_REPORTERS_HPP__

#include "msdp_parser.h"

#include <memory>

#include "engine/network/descriptor_data.h"

class CharData;        // to avoid inclusion of "char.hpp"

namespace msdp {
class AbstractReporter {
 public:
	using shared_ptr = std::shared_ptr<AbstractReporter>;

	virtual void get(Variable::shared_ptr &response) = 0;

	virtual ~AbstractReporter() {}
};

class DescriptorBasedReporter : public AbstractReporter {
 public:
	DescriptorBasedReporter(const DescriptorData *descriptor) : m_descriptor(descriptor) {}

 protected:
	const auto descriptor() const { return m_descriptor; }

 private:
	const DescriptorData *m_descriptor;
};

class RoomReporter : public DescriptorBasedReporter {
 public:
	RoomReporter(const DescriptorData *descriptor) : DescriptorBasedReporter(descriptor) {}

	virtual void get(Variable::shared_ptr &response) override;

	static shared_ptr create(const DescriptorData *descriptor) { return std::make_shared<RoomReporter>(descriptor); }

 private:
	bool blockReport() const;
};

class GoldReporter : public DescriptorBasedReporter {
 public:
	GoldReporter(const DescriptorData *descriptor) : DescriptorBasedReporter(descriptor) {}

	virtual void get(Variable::shared_ptr &response) override;

	static shared_ptr create(const DescriptorData *descriptor) { return std::make_shared<GoldReporter>(descriptor); }
};

class MaxHitReporter : public DescriptorBasedReporter {
 public:
	MaxHitReporter(const DescriptorData *descriptor) : DescriptorBasedReporter(descriptor) {}

	virtual void get(Variable::shared_ptr &response) override;

	static shared_ptr create(const DescriptorData *descriptor) { return std::make_shared<MaxHitReporter>(descriptor); }
};

class MaxMoveReporter : public DescriptorBasedReporter {
 public:
	MaxMoveReporter(const DescriptorData *descriptor) : DescriptorBasedReporter(descriptor) {}

	virtual void get(Variable::shared_ptr &response) override;

	static shared_ptr create(const DescriptorData *descriptor) { return std::make_shared<MaxMoveReporter>(descriptor); }
};

class MaxManaReporter : public DescriptorBasedReporter {
 public:
	MaxManaReporter(const DescriptorData *descriptor) : DescriptorBasedReporter(descriptor) {}

	virtual void get(Variable::shared_ptr &response) override;

	static shared_ptr create(const DescriptorData *descriptor) { return std::make_shared<MaxManaReporter>(descriptor); }
};

// Репортёры принятых в MSDP имён. Данные берутся оттуда же, откуда их берут наши
// исторические переменные; отдельные классы нужны потому, что в ответе стоит то имя, которым
// спросили, -- клиент сопоставляет ответ с запросом по имени.
// MANA и MANA_MAX молчат у тех, кто маной не колдует: промолчавший репортёр -- не ошибка.
class HealthReporter : public DescriptorBasedReporter {
 public:
	HealthReporter(const DescriptorData *descriptor) : DescriptorBasedReporter(descriptor) {}

	virtual void get(Variable::shared_ptr &response) override;

	static shared_ptr create(const DescriptorData *descriptor) { return std::make_shared<HealthReporter>(descriptor); }
};

class HealthMaxReporter : public DescriptorBasedReporter {
 public:
	HealthMaxReporter(const DescriptorData *descriptor) : DescriptorBasedReporter(descriptor) {}

	virtual void get(Variable::shared_ptr &response) override;

	static shared_ptr create(const DescriptorData *descriptor) { return std::make_shared<HealthMaxReporter>(descriptor); }
};

class MovementReporter : public DescriptorBasedReporter {
 public:
	MovementReporter(const DescriptorData *descriptor) : DescriptorBasedReporter(descriptor) {}

	virtual void get(Variable::shared_ptr &response) override;

	static shared_ptr create(const DescriptorData *descriptor) { return std::make_shared<MovementReporter>(descriptor); }
};

class MovementMaxReporter : public DescriptorBasedReporter {
 public:
	MovementMaxReporter(const DescriptorData *descriptor) : DescriptorBasedReporter(descriptor) {}

	virtual void get(Variable::shared_ptr &response) override;

	static shared_ptr create(const DescriptorData *descriptor) { return std::make_shared<MovementMaxReporter>(descriptor); }
};

class ManaReporter : public DescriptorBasedReporter {
 public:
	ManaReporter(const DescriptorData *descriptor) : DescriptorBasedReporter(descriptor) {}

	virtual void get(Variable::shared_ptr &response) override;

	static shared_ptr create(const DescriptorData *descriptor) { return std::make_shared<ManaReporter>(descriptor); }
};

class ManaMaxReporter : public DescriptorBasedReporter {
 public:
	ManaMaxReporter(const DescriptorData *descriptor) : DescriptorBasedReporter(descriptor) {}

	virtual void get(Variable::shared_ptr &response) override;

	static shared_ptr create(const DescriptorData *descriptor) { return std::make_shared<ManaMaxReporter>(descriptor); }
};

class LevelReporter : public DescriptorBasedReporter {
 public:
	LevelReporter(const DescriptorData *descriptor) : DescriptorBasedReporter(descriptor) {}

	virtual void get(Variable::shared_ptr &response) override;

	static shared_ptr create(const DescriptorData *descriptor) { return std::make_shared<LevelReporter>(descriptor); }
};

class ExperienceReporter : public DescriptorBasedReporter {
 public:
	ExperienceReporter(const DescriptorData *descriptor) : DescriptorBasedReporter(descriptor) {}

	virtual void get(Variable::shared_ptr &response) override;

	static shared_ptr create(const DescriptorData *descriptor) { return std::make_shared<ExperienceReporter>(descriptor); }
};

class StateReporter : public DescriptorBasedReporter {
 public:
	StateReporter(const DescriptorData *descriptor) : DescriptorBasedReporter(descriptor) {}

	virtual void get(Variable::shared_ptr &response) override;

	static shared_ptr create(const DescriptorData *descriptor) { return std::make_shared<StateReporter>(descriptor); }
};

class GroupReporter : public DescriptorBasedReporter {
 public:
	GroupReporter(const DescriptorData *descriptor) : DescriptorBasedReporter(descriptor) {}

	virtual void get(Variable::shared_ptr &response) override;

	static shared_ptr create(const DescriptorData *descriptor) { return std::make_shared<GroupReporter>(descriptor); }

 private:
	void append_char(const std::shared_ptr<ArrayValue> &group,
					 const CharData *ch,
					 const CharData *character,
					 const bool leader);

	int get_mem(const CharData *character) const;
};
}

#endif // __MSDP_REPORTERS_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
