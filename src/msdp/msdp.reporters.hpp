#ifndef __MSDP_REPORTERS_HPP__
#define __MSDP_REPORTERS_HPP__

#include "msdp.parser.hpp"

#include <memory>

struct DESCRIPTOR_DATA;	// to avoid inclusion of "structs.h"
class CHAR_DATA;		// to avoid inclusion of "char.hpp"

namespace msdp
{
	class AbstractReporter
	{
	public:
		using shared_ptr = std::shared_ptr<AbstractReporter>;

		virtual void get(Variable::shared_ptr& response) = 0;

		virtual ~AbstractReporter() {}
	};

	class DescriptorBasedReporter : public AbstractReporter
	{
	public:
		DescriptorBasedReporter(const DESCRIPTOR_DATA* descriptor) : m_descriptor(descriptor) {}

	protected:
		const auto descriptor() const { return m_descriptor; }

	private:
		const DESCRIPTOR_DATA* m_descriptor;
	};

	class RoomReporter : public DescriptorBasedReporter
	{
	public:
		RoomReporter(const DESCRIPTOR_DATA* descriptor) : DescriptorBasedReporter(descriptor) {}

		virtual void get(Variable::shared_ptr& response) override;

		static shared_ptr create(const DESCRIPTOR_DATA* descriptor) { return std::make_shared<RoomReporter>(descriptor); }

	private:
		bool blockReport() const;
	};

	class GoldReporter : public DescriptorBasedReporter
	{
	public:
		GoldReporter(const DESCRIPTOR_DATA* descriptor) : DescriptorBasedReporter(descriptor) {}

		virtual void get(Variable::shared_ptr& response) override;

		static shared_ptr create(const DESCRIPTOR_DATA* descriptor) { return std::make_shared<GoldReporter>(descriptor); }
	};

	class MaxHitReporter : public DescriptorBasedReporter
	{
	public:
		MaxHitReporter(const DESCRIPTOR_DATA* descriptor) : DescriptorBasedReporter(descriptor) {}

		virtual void get(Variable::shared_ptr& response) override;

		static shared_ptr create(const DESCRIPTOR_DATA* descriptor) { return std::make_shared<MaxHitReporter>(descriptor); }
	};

	class MaxMoveReporter : public DescriptorBasedReporter
	{
	public:
		MaxMoveReporter(const DESCRIPTOR_DATA* descriptor) : DescriptorBasedReporter(descriptor) {}

		virtual void get(Variable::shared_ptr& response) override;

		static shared_ptr create(const DESCRIPTOR_DATA* descriptor) { return std::make_shared<MaxMoveReporter>(descriptor); }
	};

	class MaxManaReporter : public DescriptorBasedReporter
	{
	public:
		MaxManaReporter(const DESCRIPTOR_DATA* descriptor) : DescriptorBasedReporter(descriptor) {}

		virtual void get(Variable::shared_ptr& response) override;

		static shared_ptr create(const DESCRIPTOR_DATA* descriptor) { return std::make_shared<MaxManaReporter>(descriptor); }
	};

	class LevelReporter : public DescriptorBasedReporter
	{
	public:
		LevelReporter(const DESCRIPTOR_DATA* descriptor) : DescriptorBasedReporter(descriptor) {}

		virtual void get(Variable::shared_ptr& response) override;

		static shared_ptr create(const DESCRIPTOR_DATA* descriptor) { return std::make_shared<LevelReporter>(descriptor); }
	};

	class ExperienceReporter : public DescriptorBasedReporter
	{
	public:
		ExperienceReporter(const DESCRIPTOR_DATA* descriptor) : DescriptorBasedReporter(descriptor) {}

		virtual void get(Variable::shared_ptr& response) override;

		static shared_ptr create(const DESCRIPTOR_DATA* descriptor) { return std::make_shared<ExperienceReporter>(descriptor); }
	};

	class StateReporter : public DescriptorBasedReporter
	{
	public:
		StateReporter(const DESCRIPTOR_DATA* descriptor) : DescriptorBasedReporter(descriptor) {}

		virtual void get(Variable::shared_ptr& response) override;

		static shared_ptr create(const DESCRIPTOR_DATA* descriptor) { return std::make_shared<StateReporter>(descriptor); }
	};

	class GroupReporter : public DescriptorBasedReporter
	{
	public:
		GroupReporter(const DESCRIPTOR_DATA* descriptor) : DescriptorBasedReporter(descriptor) {}

		virtual void get(Variable::shared_ptr& response) override;

		static shared_ptr create(const DESCRIPTOR_DATA* descriptor) { return std::make_shared<GroupReporter>(descriptor); }

	private:
		void append_char(const std::shared_ptr<ArrayValue>& group, const CHAR_DATA* ch, const CHAR_DATA* character, const bool leader);

		int get_mem(const CHAR_DATA* character) const;
	};
}

#endif // __MSDP_REPORTERS_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
