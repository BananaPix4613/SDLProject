#pragma once

#include <string>
#include <functional>

/**
 * @class Command
 * @brief Base class for all undoable operations in the editor
 * 
 * The Command class implements the Command pattern to encapsulate
 * editor operations that can be executed, undone, and redone.
 * Each concrete command should represent a single atomic operation
 */
class Command
{
public:
    /**
     * @brief Constructor
     * @param name Description of the command for UI display
     */
    Command(const std::string& name);

    /**
     * @brief Virtual destructor
     */
    virtual ~Command();

    /**
     * @brief Execute the command
     * @return True if execution succeeded
     */
    virtual bool execute() = 0;

    /**
     * @brief Undo the command
     * @return True if undo succeeded
     */
    virtual bool undo() = 0;

    /**
     * @brief Redo the command
     * @return True if redo succeeded
     */
    virtual bool redo();

    /**
     * @brief Get the name of the command
     * @return Command name
     */
    const std::string& getName() const;

    /**
     * @brief Check if command can be merged with another command
     * @param other Command to check for merge compatibility
     * @return True if commands can be merged
     */
    virtual bool canMergeWith(const Command* other) const;

    /**
     * @brief Merge another command into this one
     * @param other Command to merge into this one
     * @return True if merge succeeded
     */
    virtual bool mergeWith(const Command* other);

    /**
     * @brief Check if command should be added to history
     * @return True if command should be added to history
     */
    bool shouldAddToHistory() const;

    /**
     * @brief Set whether command should be added to history
     * @param addToHistory Whether command should be added to history
     */
    void setShouldAddToHistory(bool addToHistory);

protected:
    std::string m_name;
    bool m_addToHistory;
};

/**
 * @class LambdaCommand
 * @brief Command implementation using lambda functions
 * 
 * This class enables quick creation of commands using lambda functions
 * for execute, undo, and redo operations.
 */
class LambdaCommand : public Command
{
public:
    /**
     * @brief Constructor
     * @param name Command name
     * @param executeFunc Function to execute the command
     * @param undoFunc Function to undo the command
     */
    LambdaCommand(const std::string& name,
                  std::function<bool()> executeFunc,
                  std::function<bool()> undoFunc);

    /**
     * @brief Execute the command
     * @return True if execution succeeded
     */
    bool execute() override;

    /**
     * @brief Undo the command
     * @return True if undo succeeded
     */
    bool undo() override;

private:
    std::function<bool()> m_executeFunc;
    std::function<bool()> m_undoFunc;
};