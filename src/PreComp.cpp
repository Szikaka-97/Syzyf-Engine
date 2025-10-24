#include <PreComp.h>

const std::regex Regex::shaderUniformRegex("(layout\\s*\\(\\s*location\\s*=\\s*(\\d+)\\s*\\)\\s*)?uniform\\s+((mat[2-4](x[2-4])?)|((b|i|u|d)?vec[2-4])|(bool|float|int|uint|double))\\s+([_a-zA-Z][a-zA-Z0-9]*)(\\[\\d+\\])?(\\s*=\\s*.+)?;");
const std::regex Regex::shaderInputRegex("layout\\s*\\((IN_(POSITION|NORMAL|BINORMAL|TANGENT|UV1|UV2|COLOR))\\)\\s*in\\s+(float|vec[2-4])\\s+([a-zA-Z][a-zA-Z0-9]*)\\s*;");
const std::regex Regex::shaderHeaderRegex("#version.*\\n");