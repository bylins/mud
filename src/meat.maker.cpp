#include "meat.maker.hpp"

#include "random.hpp"

const obj_vnum MeatMapping::ARTEFACT_KEY;

const MeatMapping::raw_mapping_t MeatMapping::RAW_MAPPING = {
	meat_mapping_t(320, 334),
	meat_mapping_t(321, 335),
	meat_mapping_t(322, 336),
	meat_mapping_t(323, 337)
};

MeatMapping::MeatMapping()
{
	for (const auto& mapping : RAW_MAPPING)
	{
		emplace(mapping);
	}
	build_randomly_returnable_keys_index();

	emplace(ARTEFACT_KEY, 338);
}

MeatMapping::key_type MeatMapping::random_key() const
{
	const auto index = number(0, static_cast<int>(m_randomly_returnable_keys.size() - 1));
	//	sprintf(buf, "Размер мясного массива %d выпал предмет под номером %d с vnum %d", static_cast<int>(size()), index, m_index_mapping[index]);
	//	mudlog(buf, NRM, LVL_IMMORT, SYSLOG, TRUE);
	return m_randomly_returnable_keys[index];
}

void MeatMapping::build_randomly_returnable_keys_index()
{
	size_t i = 0;
	m_randomly_returnable_keys.resize(size());
	for (const auto& p : *this)
	{
		m_randomly_returnable_keys[i++] = p.first;
	}
}

MeatMapping meat_mapping;

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
