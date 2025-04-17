#include "Command.h"

Command::Command(const std::string& name) :
    m_name(name), m_addToHistory(true)
{
}

Command::~Command()
{
}

bool Command::redo()
{
    // By default, redo just executes the command again
    return execute();
}

const std::string& Command::getName() const
{
    return m_name;
}

bool Command::canMergeWith(const Command* other) const
{
    // By default, commands cannot be merged
    return false;
}

bool Command::mergeWith(const Command* other)
{
    // By default, merging is not implemented
    return false;
}

bool Command::shouldAddToHistory() const
{
    return m_addToHistory;
}

void Command::setShouldAddToHistory(bool addToHistory)
{
    m_addToHistory = addToHistory;
}

// Lambda Command implementation
LambdaCommand::LambdaCommand(const std::string& name,
                             std::function<bool()> executeFunc,
                             std::function<bool()> undoFunc) :
    Command(name), m_executeFunc(executeFunc), m_undoFunc(undoFunc)
{
}

bool LambdaCommand::execute()
{
    if (m_executeFunc)
    {
        return m_executeFunc();
    }
    return false;
}

bool LambdaCommand::undo()
{
    if (m_undoFunc)
    {
        return m_undoFunc();
    }
    return false;
}