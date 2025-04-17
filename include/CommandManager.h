#pragma once

#include "Command.h"
#include <vector>
#include <memory>
#include <stack>
#include <string>

/**
 * @class Transaction
 * @brief A group of commands that are treated as a single operation
 * 
 * Transactions allow multiple commands to be grouped together and
 * executed, undone, and redone as a single unit.
 */
class Transaction : public Command
{
public:
    /**
     * @brief Constructor
     * @param name Name of the transaction
     */
    Transaction(const std::string& name);

    /**
     * @brief Destructor
     */
    ~Transaction() override;

    /**
     * @brief Add a command to the transaction
     * @param command Command to add
     */
    void addCommand(std::unique_ptr<Command> command);

    /**
     * @brief Execute all commands in the transaction
     * @return True if all commands are executed successfully
     */
    bool execute() override;

    /**
     * @brief Undo all commands in the transaction in reverse order
     * @return True if all commands were undone successfully
     */
    bool undo() override;

    /**
     * @brief Redo all commands in the transaction
     * @return True if all commands were redone successfully
     */
    bool redo() override;

    /**
     * @brief Get the number of commands in the transaction
     * @return Command count
     */
    size_t getCommandCount() const;

    /**
     * @brief Check if the transaction is empty
     * @return True if no commands have been added
     */
    bool isEmpty() const;

private:
    std::vector<std::unique_ptr<Command>> m_commands;
};

/**
 * @class CommandManager
 * @brief Manages command history for undo/redo functionality
 * 
 * The CommandManager maintains a history of executed commands
 * and provides methods for undoing and redoing them. It also
 * supports transactions for grouping commands.
 */
class CommandManager
{
public:
    /**
     * @brief Constructor
     */
    CommandManager();

    /**
     * @brief Destructor
     */
    ~CommandManager();

    /**
     * @brief Execute a command and add it to the history
     * @param command Command to execute
     * @return True if command was executed successfully
     */
    bool executeCommand(std::unique_ptr<Command> command);

    /**
     * @brief Begin a transaction
     * @param name Name of the transaction
     */
    void beginTransaction(const std::string& name);

    /**
     * @brief Commit the current transaction
     * @return True if transaction was commited successfully
     */
    bool commitTransaction();

    /**
     * @brief Abort the current transaction
     */
    void abortTransaction();

    /**
     * @brief Check if a transaction is in progress
     * @return True if a transaction is in progress
     */
    bool isInTransaction() const;

    /**
     * @brief Undo the last command or transaction
     * @return True if undo was successful
     */
    bool undo();

    /**
     * @brief Redo the last undone command or transaction
     * @return True if redo was successful
     */
    bool redo();

    /**
     * @brief Check if undo is available
     * @return True if there is something to undo
     */
    bool canUndo() const;

    /**
     * @brief Check if redo is available
     * @return True if there is something to redo
     */
    bool canRedo() const;

    /**
     * @brief Get the name of the command that would be undone next
     * @return Name of the next undo command, or empty string if none
     */
    std::string getUndoName() const;

    /**
     * @brief Get the name of the command that would be redone next
     * @return Name of the next redo command, or empty string if none
     */
    std::string getRedoName() const;

    /**
     * @brief Clear the command history
     */
    void clearHistory();

    /**
     * @brief Set the maximum size of the command history
     * @param maxSize Maximum number of commands to store (0 for unlimited)
     */
    void setMaxHistorySize(size_t maxSize);

    /**
     * @brief Get the current size of the command history
     * @return Number of commands in the history
     */
    size_t getHistorySize() const;

    /**
     * @brief Set whether command merging is enabled
     * @param enable Whether to enable command merging
     */
    void setCommandMergingEnabled(bool enable);

    /**
     * @brief Check if command merging is enabled
     * @return True if command merging is enabled
     */
    bool isCommandMergingEnabled() const;

private:
    std::vector<std::unique_ptr<Command>> m_undoStack;
    std::vector<std::unique_ptr<Command>> m_redoStack;

    std::stack<std::unique_ptr<Transaction>> m_transactionStack;
    size_t m_maxHistorySize;
    bool m_commandMergingEnabled;

    void clearRedoStack();
    bool tryMergeCommand(std::unique_ptr<Command>& command);
};