// Regression tests for Koi8rYamlEmitter -- see issue #3273.
//
// Before the fix, values whose first character was a YAML indicator (&, *, !, ,)
// were emitted as bare plain scalars. yaml-cpp then re-parsed them as anchors,
// aliases or tags, breaking save -> reload round-trip. MUD color codes like
// "&Yfoo &Wbar&n" in object/mob names emitted unquoted yielded
// "cannot assign multiple anchors to the same node".

#ifdef HAVE_YAML

#include "engine/db/koi8r_yaml_emitter.h"

#include <gtest/gtest.h>
#include <yaml-cpp/yaml.h>
#include <sstream>
#include <string>

namespace
{

// Emit `value` as a single mapping entry "v: <value>" via the emitter, then
// parse it back with yaml-cpp and return the scalar string.
std::string RoundTrip(const std::string &value)
{
	std::ostringstream out;
	Koi8rYamlEmitter yaml(out);
	yaml.Key("v");
	yaml.Value(value);
	YAML::Node node = YAML::Load(out.str());
	return node["v"].as<std::string>();
}

}  // namespace

TEST(Koi8rYamlEmitter, RoundTripPlainAscii)
{
	EXPECT_EQ(RoundTrip("hello"), "hello");
}

TEST(Koi8rYamlEmitter, RoundTripEmpty)
{
	EXPECT_EQ(RoundTrip(""), "");
}

// Exact scenario from issue #3273: multiple "&X..." color tokens. yaml-cpp used
// to interpret each "&" as an anchor declaration and fail with "cannot assign
// multiple anchors to the same node".
TEST(Koi8rYamlEmitter, ColorCodesAtStartRoundTrip)
{
	const std::string color_name = "&Yfoo &Wbar&n";
	EXPECT_EQ(RoundTrip(color_name), color_name);
}

// Single leading "&" also used to break (parsed as anchor, value becomes null).
TEST(Koi8rYamlEmitter, SingleLeadingAmpersandRoundTrip)
{
	EXPECT_EQ(RoundTrip("&Y"), "&Y");
}

// "*" at start of a plain scalar is an alias indicator.
TEST(Koi8rYamlEmitter, LeadingAsteriskRoundTrip)
{
	EXPECT_EQ(RoundTrip("*ref"), "*ref");
}

// "!" at start of a plain scalar is a tag indicator.
TEST(Koi8rYamlEmitter, LeadingExclamationRoundTrip)
{
	EXPECT_EQ(RoundTrip("!important"), "!important");
}

// "," at start of a plain scalar is a flow indicator.
TEST(Koi8rYamlEmitter, LeadingCommaRoundTrip)
{
	EXPECT_EQ(RoundTrip(",comma"), ",comma");
}

// Pre-existing behaviour: strings containing single quotes are escaped.
TEST(Koi8rYamlEmitter, SingleQuoteEscaped)
{
	const std::string value = "it's a quote";
	EXPECT_EQ(RoundTrip(value), value);
}

// Pre-existing behaviour: colons need quoting (otherwise "a: b" parses as map).
TEST(Koi8rYamlEmitter, ColonInValueRoundTrip)
{
	EXPECT_EQ(RoundTrip("foo: bar"), "foo: bar");
}

#endif  // HAVE_YAML

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
