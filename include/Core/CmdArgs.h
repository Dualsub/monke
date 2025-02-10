#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <optional>

namespace mk
{
    class CmdArgs
    {
    private:
        // Store both flags and options (flag is key with no value, option is key with a value)
        std::unordered_map<std::string, std::optional<std::string>> m_arguments;
        std::vector<std::string> m_positionalArgs;

        // Helper function to parse command-line arguments
        void ParseArguments(int argc, char *argv[])
        {
            std::string key;
            for (int i = 1; i < argc; ++i)
            {
                std::string arg = argv[i];
                if (arg.rfind("--", 0) == 0 || arg.rfind("-", 0) == 0)
                {
                    if (!key.empty())
                    {
                        // Previous option was a flag without a value
                        m_arguments[key] = std::nullopt;
                    }
                    key = arg;
                }
                else if (!key.empty())
                {
                    // Option with a value
                    m_arguments[key] = arg;
                    key.clear();
                }
                else
                {
                    // Positional argument
                    m_positionalArgs.push_back(arg);
                }
            }

            // Handle last flag without value
            if (!key.empty())
            {
                m_arguments[key] = std::nullopt;
            }
        }

    public:
        CmdArgs() = default;

        static CmdArgs Parse(int argc, char *argv[])
        {
            CmdArgs cmdArgs;
            cmdArgs.ParseArguments(argc, argv);
            return cmdArgs;
        }

        // Check if a flag is present (e.g., --help)
        bool HasFlag(const std::string &flag) const
        {
            return m_arguments.find(flag) != m_arguments.end();
        }

        // Get the value associated with an option (e.g., --input file.txt)
        std::string GetOptionValue(const std::string &option, const std::string &defaultValue = "") const
        {
            auto it = m_arguments.find(option);
            if (it != m_arguments.end() && it->second.has_value())
            {
                return it->second.value();
            }
            return defaultValue;
        }

        // Get a positional argument (e.g., program_name positional_arg1 positional_arg2)
        std::optional<std::string> GetPositionalArg(size_t index) const
        {
            if (index < m_positionalArgs.size())
            {
                return m_positionalArgs[index];
            }

            return std::nullopt;
        }

        // Get the number of positional arguments
        size_t GetPositionalArgCount() const
        {
            return m_positionalArgs.size();
        }
    };
}