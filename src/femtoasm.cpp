
#include <array>
#include <bit>
#include <cstdint>
#include <exception>
#include <format>
#include <iostream>
#include <limits>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <string_view>

// FIXME: this program won't work on Windows due to newline conversion

struct Position
{
	int lineNumber = 1;
	int columnNumber = 0;
};

struct Value
{
	std::uint64_t value;
	int length = 0;
};

struct Definition
{
	std::string identifier;
	Value value;
};

using Definitions = std::array<Definition, 1 << 16>;

struct EndOfFile : std::runtime_error
{
	EndOfFile() : std::runtime_error("unexpected end of file")
	{
	}
};

bool isSeparator(char c)
{
	return c >= '\0' && c <= ' ';
}

std::optional<unsigned> hexCharacterToInteger(char character)
{
	if (character >= '0' && character <= '9')
	{
		return character - '0';
	}
	if (character >= 'A' && character <= 'F')
	{
		return character - 'A' + 0xA;
	}
	if (isSeparator(character))
	{
		return std::nullopt;
	}
	throw std::runtime_error(std::format("'{}' is not a valid character for a value", character));
}

std::uint16_t hash(std::string_view identifier, std::uint16_t previous = 0)
{
	auto fn = [](auto accum, auto c)
	{
		return std::byteswap(accum) ^ c;
	};

	return std::accumulate(identifier.begin(), identifier.end(), previous, fn);
}

char readNextCharacter(Position& position)
{
	int character = std::cin.get();

	if (character == decltype(std::cin)::traits_type::eof())
	{
		throw EndOfFile();
	}

	if (!std::cin)
	{
		throw std::runtime_error("failed to read from stdin");
	}

	if (character == '\n')
	{
		position.lineNumber++;
		position.columnNumber = 0;
	}
	else
	{
		position.columnNumber++;
	}

	return static_cast<char>(character);
}

void write(Value value)
{
	if constexpr (std::endian::native == std::endian::big)
	{
		value.value = std::byteswap(value.value);
	}

	auto bytes = std::bit_cast<std::array<char, 8>>(value.value);
	std::cout.write(bytes.data(), value.length);

	if (!std::cout)
	{
		throw std::runtime_error("failed to write to stdout");
	}
}

std::string parseIdentifier(Position& position)
{
	try
	{
		std::string identifier;

		char character;

		while (!isSeparator(character = readNextCharacter(position)))
		{
			identifier.push_back(character);
		}

		if (identifier.empty())
		{
			throw std::runtime_error("identifier cannot be empty");
		}

		return identifier;
	}
	catch (...)
	{
		std::throw_with_nested(std::runtime_error("in identifier"));
	}
}

Value parseValue(Position& position)
{
	try
	{
		Value value;

		while (auto digit = hexCharacterToInteger(readNextCharacter(position)))
		{
			value.value = (value.value << 4) | *digit;
			value.length++;
		}

		if (value.length == 0 || value.length % 2 != 0 || value.length > 16)
		{
			throw std::runtime_error("invalid value length");
		}

		value.length /= 2;

		return value;
	}
	catch (...)
	{
		std::throw_with_nested(std::runtime_error("in value"));
	}
}

void parseAndStoreDefine(Definitions& definitions, Position& position)
{
	try
	{
		auto identifier = parseIdentifier(position);
		auto value = parseValue(position);

		if (value.length > 4)
		{
			throw std::runtime_error("definitions are limited to 4 bytes");
		}

		auto& slot = definitions.at(hash(identifier));

		if (slot.value.length != 0)
		{
			if (identifier == slot.identifier)
			{
				throw std::runtime_error(std::format("'{}' redefined", identifier));
			}

			throw std::runtime_error(
				std::format("hash conflict between '{}' and '{}'", slot.identifier, identifier));
		}

		slot.identifier = identifier;
		slot.value = value;
	}
	catch (...)
	{
		std::throw_with_nested(std::runtime_error("in definition"));
	}
}

void parseAndLoadAndWriteConstant(Definitions& definitions, Position& position)
{
	try
	{
		auto identifier = parseIdentifier(position);
		auto& slot = definitions.at(hash(identifier));

		if (slot.value.length == 0)
		{
			throw std::runtime_error(std::format("'{}' is undefined", identifier));
		}

		write(slot.value);
	}
	catch (...)
	{
		std::throw_with_nested(std::runtime_error("in constant"));
	}
}

void parseAndWriteLiteral(Position& position)
{
	try
	{
		write(parseValue(position));
	}
	catch (...)
	{
		std::throw_with_nested(std::runtime_error("in literal"));
	}
}

void femtoasm(int argc, char**)
{
	if (argc != 1)
	{
		throw std::runtime_error("femtoasm expects no arguments");
	}

	Position position;
	Definitions definitions;

	try
	{
		while (true)
		{
			char character;

			try
			{
				character = readNextCharacter(position);
			}
			catch (const EndOfFile&)
			{
				break;
			}

			switch (character)
			{
			case '=':
				parseAndStoreDefine(definitions, position);
				break;

			case '$':
				parseAndLoadAndWriteConstant(definitions, position);
				break;

			case '%':
				parseAndWriteLiteral(position);
				break;
			}
		}
	}
	catch (...)
	{
		std::throw_with_nested(std::runtime_error(
			std::format("at stdin:{}:{}", position.lineNumber, position.columnNumber)));
	}
}

void print(const std::exception& e)
{
	std::cerr << ": " << e.what();
	try
	{
		std::rethrow_if_nested(e);
	}
	catch (const std::exception& ex)
	{
		print(ex);
	}
}

int main(int argc, char** argv)
{
	try
	{
		femtoasm(argc, argv);
		return 0;
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error";
		print(e);
		std::cerr << "\n";
		return 1;
	}
}
