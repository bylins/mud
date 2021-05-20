#if !defined __XML_LOADING_HELPER_HPP__
#define __XML_LOADING_HELPER_HPP__

#include "structs.h"
#include "utils.h"
#include "pugixml.hpp"

namespace xml
{
	namespace loading
	{
		class CHelper
		{
		private:
			template <typename EnumType>
			static void set_bit(FLAG_DATA& flags, const EnumType flag) { flags.set(flag); }
			template <typename EnumType>
			static void set_bit(uint32_t& flags, const EnumType flag) { SET_BIT(flags, flag); }

		public:
			enum ELoadFlagResult
			{
				ELFR_SUCCESS,
				ELFR_NO_VALUE,
				ELFR_FAIL
			};

			template <class TFlags, typename TSuccessHandler, typename TFailHandler, typename TFlagsSetter>
			static void load_flags(const pugi::xml_node& root, const char* node_name, const char* node_flag,
				const TFlagsSetter setter, const TSuccessHandler success_handler, const TFailHandler fail_handler);
			template <class TFlags, typename TSuccessHandler, typename TFailHandler, typename TFlagsStorage>
			static void load_flags(TFlagsStorage flags, const pugi::xml_node& root, const char* node_name, const char* node_flag,
				const TSuccessHandler success_handler, const TFailHandler fail_handler);

			template <class TFlag, typename TSuccessHandler, typename TFailHandler, typename TNoValueHandler>
			static ELoadFlagResult load_flag(const pugi::xml_node& root, const char* node_name,
				const TSuccessHandler success_handler, const TFailHandler fail_handler, const TNoValueHandler no_value_handler);

			template <typename TCatchHandler>
			static void load_integer(const char* input, int& output, const TCatchHandler catch_handler);

			template <typename TSetHandler, typename TCatchHandler>
			static void load_integer(const char* input, const TSetHandler set_handler, const TCatchHandler catch_handler);

			template <typename KeyType,
				typename TNoKeyHandler,
				typename TKeyConverter,
				typename TConverFailHandler,
				typename TNoValueHandler,
				typename TSetHandler>
				static void load_pairs_list(
					const pugi::xml_node* node,
					const char* group_name,
					const char* entry_name,
					const char* key_name,
					const char* value_name,
					const TNoKeyHandler no_key_handler,
					const TKeyConverter key_converter,
					const TConverFailHandler convert_fail_handler,
					const TNoValueHandler no_value_handler,
					const TSetHandler set_handler);

			template <typename TFailHandler>
			static void save_string(pugi::xml_node& node, const char* node_name, const char* value, const TFailHandler fail_handler);

			template <typename ListType, typename TGetItemValue, typename TListNodeFailHandler, typename TItemNodeFailHandler>
			static void save_list(
				pugi::xml_node& node,
				const char* node_name,
				const char* item_name,
				const ListType& list,
				const TGetItemValue get_item_value,
				const TListNodeFailHandler list_node_fail_handler,
				const TItemNodeFailHandler item_node_fail_handler);

			template <typename ListType, typename TListNodeFailHandler, typename TItemNodeFailHandler>
			static void save_list(
				pugi::xml_node& node,
				const char* node_name,
				const char* item_name,
				const ListType& list,
				const TListNodeFailHandler list_node_fail_handler,
				const TItemNodeFailHandler item_node_fail_handler)
			{
				save_list(node, node_name, item_name, list,
					[&](const auto value) -> auto { return NAME_BY_ITEM(value).c_str(); },
					list_node_fail_handler,
					item_node_fail_handler);
			}

			template <typename FlagType, typename TListNodeFailHandler, typename TItemNodeFailHandler>
			static void save_list(
				pugi::xml_node& node,
				const char* node_name,
				const char* item_name,
				const FLAG_DATA& flags,
				const TListNodeFailHandler list_node_fail_handler,
				const TItemNodeFailHandler item_node_fail_handler);

			template <typename FlagType, typename TListNodeFailHandler, typename TItemNodeFailHandler>
			static void save_list(
				pugi::xml_node& node,
				const char* node_name,
				const char* item_name,
				const uint32_t flags,
				const TListNodeFailHandler list_node_fail_handler,
				const TItemNodeFailHandler item_node_fail_handler)
			{
				std::list<FlagType> list;
				uint32_t flag = 1;
				while (flag < flags)
				{
					if (IS_SET(flags, flag))
					{
						list.push_back(static_cast<FlagType>(flag));
					}
					flag <<= 1;
				}

				save_list(node, node_name, item_name, list,
					list_node_fail_handler,
					item_node_fail_handler);
			}

			template <typename ListType, typename TGetKeyHandler, typename TGetValueHandler, typename TListNodeFailHandler, typename TItemFailHandler>
			static void save_pairs_list(
				pugi::xml_node& node,
				const char* node_name,
				const char* item_node_name,
				const char* key_node_name,
				const char* value_node_name,
				const ListType& list,
				const TGetKeyHandler get_key_handler,
				const TGetValueHandler get_value_handler,
				const TListNodeFailHandler list_node_fail_handler,
				const TItemFailHandler item_fail_handler)
			{
				if (list.empty())
				{
					return;
				}

				pugi::xml_node* list_node = NULL;
				pugi::xml_node storage;

				for (const auto& i : list)
				{
					try
					{
						const std::string key = get_key_handler(i);
						if (key.empty())
						{
							continue;
						}

						if (NULL == list_node)
						{
							storage = node.append_child(node_name);
							list_node = &storage;
							if (!*list_node)
							{
								log("WARNING: Couldn't create node \"%s\".\n", node_name);
								list_node_fail_handler();
								return;
							}
						}

						auto item_node = list_node->append_child(item_node_name);
						if (!item_node)
						{
							log("WARNING: Could not create item node. Will be skipped.\n");
							item_fail_handler(i);
							continue;
						}

						save_string(item_node, key_node_name, key.c_str(),
							[&]() { throw std::runtime_error("failed to save key"); });
						save_string(item_node, value_node_name, get_value_handler(i).c_str(),
							[&]() { throw std::runtime_error("failed to save value"); });
					}
					catch (...)
					{
						item_fail_handler(i);
					}
				}
			}

			template <typename ListType, typename TGetKeyHandler, typename TGetValueHandler, typename TListNodeFailHandler>
			static void save_pairs_list(
				pugi::xml_node& node,
				const char* node_name,
				const char* item_node_name,
				const char* key_node_name,
				const char* value_node_name,
				const ListType& list,
				const TGetKeyHandler get_key_handler,
				const TGetValueHandler get_value_handler,
				const TListNodeFailHandler list_node_fail_handler)
			{
				save_pairs_list(node, node_name, item_node_name, key_node_name, value_node_name, list,
					get_key_handler, get_value_handler, list_node_fail_handler, [](const auto) {});
			}
		};

		template <class TFlags, typename TSuccessHandler, typename TFailHandler, typename TFlagsSetter>
		void CHelper::load_flags(const pugi::xml_node& root, const char* node_name, const char* node_flag,
			const TFlagsSetter setter, const TSuccessHandler success_handler, const TFailHandler fail_handler)
		{
			const auto node = root.child(node_name);
			if (node)
			{
				for (const auto flag : node.children(node_flag))
				{
					const char* flag_value = flag.child_value();
					try
					{
						auto value = ITEM_BY_NAME<TFlags>(flag_value);
						setter(value);
						success_handler(value);
					}
					catch (...)
					{
						fail_handler(flag_value);
					}
				}
			}
		}

		template <class TFlags, typename TSuccessHandler, typename TFailHandler, typename TFlagsStorage>
		void CHelper::load_flags(TFlagsStorage flags, const pugi::xml_node& root, const char* node_name,
			const char* node_flag, const TSuccessHandler success_handler, const TFailHandler fail_handler)
		{
			load_flags<TFlags>(root, node_name, node_flag, [&](const auto flag) { set_bit(flags, flag); }, success_handler, fail_handler);
		}

		template <class TFlag, typename TSuccessHandler, typename TFailHandler, typename TNoValueHandler>
		CHelper::ELoadFlagResult CHelper::load_flag(const pugi::xml_node& root, const char* node_name,
			const TSuccessHandler success_handler, const TFailHandler fail_handler, const TNoValueHandler no_value_handler)
		{
			const auto node = root.child(node_name);
			if (node)
			{
				const char* value = node.child_value();
				try
				{
					const TFlag type = ITEM_BY_NAME<TFlag>(value);
					success_handler(type);
				}
				catch (...)
				{
					fail_handler(value);
					return ELFR_FAIL;
				}
			}
			else
			{
				no_value_handler();
				return ELFR_NO_VALUE;
			}

			return ELFR_SUCCESS;
		}

		template <typename TCatchHandler>
		void CHelper::load_integer(const char* input, int& output, const TCatchHandler catch_handler)
		{
			try
			{
				output = std::stoi(input);
			}
			catch (...)
			{
				catch_handler();
			}
		}

		template <typename TSetHandler, typename TCatchHandler>
		void CHelper::load_integer(const char* input, const TSetHandler set_handler, const TCatchHandler catch_handler)
		{
			try
			{
				set_handler(std::stoi(input));
			}
			catch (...)
			{
				catch_handler();
			}
		}

		template <typename KeyType,
			typename TNoKeyHandler,
			typename TKeyConverter,
			typename TConverFailHandler,
			typename TNoValueHandler,
			typename TSetHandler>
			void CHelper::load_pairs_list(
				const pugi::xml_node* node,
				const char* group_name,
				const char* entry_name,
				const char* key_name,
				const char* value_name,
				const TNoKeyHandler no_key_handler,
				const TKeyConverter key_converter,
				const TConverFailHandler convert_fail_handler,
				const TNoValueHandler no_value_handler,
				const TSetHandler set_handler)
		{
			const auto group_node = node->child(group_name);
			if (!group_node)
			{
				return;
			}

			size_t number = 0;
			for (const auto entry_node : group_node.children(entry_name))
			{
				++number;
				const auto key_node = entry_node.child(key_name);
				if (!key_node)
				{
					no_key_handler(number);
					continue;
				}

				KeyType key;
				try
				{
					key = key_converter(key_node.child_value());
				}
				catch (...)
				{
					convert_fail_handler(key_node.child_value());
					continue;
				}

				const auto value_node = entry_node.child(value_name);
				if (!value_node)
				{
					no_value_handler(key_node.child_value());
					continue;
				}

				set_handler(key, value_node.child_value());
			}
		}

		template <typename TFailHandler>
		void CHelper::save_string(pugi::xml_node& node, const char* node_name, const char* value, const TFailHandler fail_handler)
		{
			auto new_node = node.append_child(node_name);
			if (!new_node)
			{
				log("WARNING: Failed to create node \"%s\".\n", node_name);
				fail_handler();
			}

			auto cdate_node = new_node.append_child(pugi::node_pcdata);
			if (!cdate_node)
			{
				log("WARNING: Could not add PCDATA child node to node \"%s\".\n", node_name);
				fail_handler();
			}

			if (!cdate_node.set_value(value))
			{
				log("WARNING: Failed to set value to node \"%s\".\n", node_name);
				fail_handler();
			}
		}

		template <typename ListType, typename TGetItemValue, typename TListNodeFailHandler, typename TItemNodeFailHandler>
		void CHelper::save_list(pugi::xml_node& node, const char* node_name, const char* item_name, const ListType& list, const TGetItemValue get_item_value, const TListNodeFailHandler list_node_fail_handler, const TItemNodeFailHandler item_node_fail_handler)
		{
			if (list.empty())
			{
				return;
			}

			auto list_node = node.append_child(node_name);
			if (!node_name)
			{
				list_node_fail_handler();
			}

			for (const auto& i : list)
			{
				try
				{
					save_string(list_node, item_name, get_item_value(i), [&]() { throw std::runtime_error("failed to save item value"); });
				}
				catch (...)
				{
					item_node_fail_handler(i);
				}
			}
		}

		template <typename FlagType, typename TListNodeFailHandler, typename TItemNodeFailHandler>
		void CHelper::save_list(pugi::xml_node& node, const char* node_name, const char* item_name, const FLAG_DATA& flags, const TListNodeFailHandler list_node_fail_handler, const TItemNodeFailHandler item_node_fail_handler)
		{
			std::list<FlagType> list;
			for (uint32_t i = 0; i < FLAG_DATA::PLANES_NUMBER; ++i)
			{
				const auto plane = flags.get_plane(i);
				for (uint32_t j = 0; j < FLAG_DATA::PLANE_SIZE; ++j)
				{
					if (IS_SET(plane, 1 << j))
					{
						const uint32_t flag_bit = (i << 30) | (1 << j);
						list.push_back(static_cast<FlagType>(flag_bit));
					}
				}
			}

			save_list(node, node_name, item_name, list,
				list_node_fail_handler,
				item_node_fail_handler);
		}
	}
}

#endif // __XML_LOADING_HELPER_HPP__
