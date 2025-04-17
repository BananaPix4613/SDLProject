#include "CommandManager.h"
#include <algorithm>

// Transaction implementation
Transaction::Transaction(const std::string& name) :
    Command(name)
{
}

Transaction::~Transaction()
{
}

void Transaction::addCommand(std::unique_ptr<Command> command)
{
    m_commands.push_back(std::move(command));
}

bool Transaction::execute()
{
    bool success = true;
    for (auto& command : m_commands)
    {
        if (!command->execute())
        {
            success = false;
            break;
        }
    }
    return success;
}

bool Transaction::undo()
{
    bool success = true;
    // Undo commands in reverse order
    for (auto it = m_commands.rbegin(); it != m_commands.rend(); it++)
    {
        if (!(*it)->undo())
        {
            success = false;
            break;
        }
    }
    return success;
}

bool Transaction::redo()
{
    bool success = true;
    // Redo commands in original order
    for (auto& command : m_commands)
    {
        if (!command->redo())
        {
            success = false;
            break;
        }
    }
    return success;
}

size_t Transaction::getCommandCount() const
{
    return m_commands.size();
}

bool Transaction::isEmpty() const
{
    return m_commands.empty();
}

// CommandManager Implementation
CommandManager::CommandManager() :
    m_maxHistorySize(100), m_commandMergingEnabled(true)
{
}

CommandManager::~CommandManager()
{
}

bool CommandManager::executeCommand(std::unique_ptr<Command> command)
{
    if (!command)
        return false;

    // Execute the command
    bool success = command->execute();

    if (success && command->shouldAddToHistory())
    {
        // Check if we're in a transaction
        if (isInTransaction())
        {
            // Add the command to the current transaction
            m_transactionStack.top()->addCommand(std::move(command));
        }
        else
        {
            // Try to merge with the last command in the undo stack
            if (m_commandMergingEnabled && !m_undoStack.empty() &&
                tryMergeCommand(command))
            {
                // Command was merged, no need to add it
            }
            else
            {
                // Clear the redo stack when a new command is executed
                clearRedoStack();

                // Add the command to the undo stack
                m_undoStack.push_back(std::move(command));

                // Check if we need to remove old commands
                while (m_maxHistorySize > 0 && m_undoStack.size() > m_maxHistorySize)
                {
                    m_undoStack.erase(m_undoStack.begin());
                }
            }
        }
    }

    return success;
}

void CommandManager::beginTransaction(const std::string& name)
{
    m_transactionStack.push(std::make_unique<Transaction>(name));
}

bool CommandManager::commitTransaction()
{
    if (!isInTransaction)
        return false;

    // Get the current transaction
    std::unique_ptr<Transaction> transaction = std::move(m_transactionStack.top());
    m_transactionStack.pop();

    // If the transaction is empty, just discard it
    if (transaction->isEmpty())
        return true;

    // If there's still a parent transaction, add this transaction to it
    if (isInTransaction())
    {
        m_transactionStack.top()->addCommand(std::move(transaction));
    }
    else
    {
        // Add the transaction to the undo stack
        clearRedoStack();
        m_undoStack.push_back(std::move(transaction));

        // Check if we need to remove old commands
        while (m_maxHistorySize > 0 && m_undoStack.size() > m_maxHistorySize)
        {
            m_undoStack.erase(m_undoStack.begin());
        }
    }

    return true;
}

void CommandManager::abortTransaction()
{
    if (!isInTransaction)
        return;

    // Get the current transaction
    std::unique_ptr<Transaction> transaction = std::move(m_transactionStack.top());
    m_transactionStack.pop();

    // Undo all commands in the transaction in reverse order
    transaction->undo();

    // Discard the transaction
    // (it will be automatically deleted when the unique_ptr goes out of scope)
}

bool CommandManager::isInTransaction() const
{
    return !m_transactionStack.empty();
}

bool CommandManager::undo()
{
    if (!canUndo())
        return false;

    // Get the last command from the undo stack
    std::unique_ptr<Command> command = std::move(m_undoStack.back());
    m_undoStack.pop_back();

    // Undo the command
    bool success = command->undo();

    if (success)
    {
        // Add the command to the redo stack
        m_redoStack.push_back(std::move(command));
    }
    else
    {
        // If undo failed, put the command back on the undo stack
        m_undoStack.push_back(std::move(command));
    }

    return success;
}

bool CommandManager::redo()
{
    if (!canRedo())
        return false;

    // Get the last command from the redo stack
    std::unique_ptr<Command> command = std::move(m_redoStack.back());
    m_redoStack.pop_back();

    // Redo the command
    bool success = command->redo();

    if (success)
    {
        // Add the command to the undo stack
        m_undoStack.push_back(std::move(command));
    }
    else
    {
        // If redo failed, put the command back on the redo stack
        m_redoStack.push_back(std::move(command));
    }

    return success;
}

bool CommandManager::canUndo() const
{
    return !m_undoStack.empty();
}

bool CommandManager::canRedo() const
{
    return !m_redoStack.empty();
}

std::string CommandManager::getUndoName() const
{
    if (!canUndo())
    {
        return "";
    }
    return m_undoStack.back()->getName();
}

std::string CommandManager::getRedoName() const
{
    if (!canRedo())
    {
        return "";
    }
    return m_redoStack.back()->getName();
}

void CommandManager::clearHistory()
{
    m_undoStack.clear();
    m_redoStack.clear();
}

void CommandManager::setMaxHistorySize(size_t maxSize)
{
    m_maxHistorySize = maxSize;

    // Remove oldest commands if we exceed the new limit
    while (m_maxHistorySize > 0 && m_undoStack.size() > m_maxHistorySize)
    {
        m_undoStack.erase(m_undoStack.begin());
    }
}

size_t CommandManager::getHistorySize() const
{
    return m_undoStack.size();
}

void CommandManager::setCommandMergingEnabled(bool enable)
{
    m_commandMergingEnabled = enable;
}

bool CommandManager::isCommandMergingEnabled() const
{
    return m_commandMergingEnabled;
}

void CommandManager::clearRedoStack()
{
    m_redoStack.clear();
}

bool CommandManager::tryMergeCommand(std::unique_ptr<Command>& command)
{
    if (m_undoStack.empty())
        return false;

    Command* lastCommand = m_undoStack.back().get();

    if (lastCommand->canMergeWith(command.get()))
    {
        return lastCommand->mergeWith(command.get());
    }

    return false;
}