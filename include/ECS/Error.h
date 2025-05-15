//// -------------------------------------------------------------------------
//// Error.h
//// -------------------------------------------------------------------------
//#pragma once
//
//#include <string>
//#include <memory>
//#include <optional>
//#include <source_location>
//#include <vector>
//
//namespace PixelCraft::ECS
//{
//
//    /**
//     * @brief Error categories for ECS operations
//     * 
//     * Categorizes errors by their source/type for easier handling
//     * and debugging. Categories help in determining appropriate
//     * recovery strategies.
//     */
//    enum class ErrorCategory
//    {
//        None = 0,          ///< No error category (for successful operations)
//        Entity,            ///< Entity-related errors (creation, destruction, lookup)
//        Component,         ///< Component-related errors (add, remove, access)
//        System,            ///< System-related errors (registration, execution)
//        Serialization,     ///< Serialization/deserialization errors
//        IO,                ///< Input/output errors (file operations)
//        Memory,            ///< Memory allocation and management errors
//        Threading,         ///< Thread synchronization and access errors
//        Validation,        ///< Input validation and state validation errors
//        Unknown            ///< Uncategorized errors
//    };
//
//    /**
//     * @brief Specific error codes within categories
//     * 
//     * Provides detailed error identification for precise error handling.
//     * Error codes are grouped by hundreds for each category.
//     */
//    enum class ErrorCode
//    {
//        None = 0,          ///< No error (successful operation)
//
//        // Entity errors (100-199)
//        EntityNotFound = 100,      ///< Entity doesn't exist in registry
//        EntityAlreadyExists,       ///< Entity ID collision
//        InvalidEntityID,           ///< Invalid or corrupt entity ID
//        EntityDestroyed,           ///< Entity was previously destroyed
//
//        // Component errors (200-299)
//        ComponentNotFound = 200,   ///< Component doesn't exist on entity
//        ComponentAlreadyExists,    ///< Component already attached to entity
//        ComponentTypeMismatch,     ///< Requested wrong component type
//        ComponentPoolFull,         ///< Component pool capacity exceeded
//
//        // System errors (300-399)
//        SystemNotFound = 300,      ///< System not registered in world
//        SystemAlreadyRegistered,   ///< System already exists
//        SystemInitializationFailed, ///< System failed to initialize
//        SystemDependencyCycle,     ///< Circular dependency detected
//
//        // Serialization errors (400-499)
//        SerializationFailed = 400, ///< Failed to serialize data
//        DeserializationFailed,     ///< Failed to deserialize data
//        InvalidFormat,             ///< Data format doesn't match expected
//        VersionMismatch,           ///< Schema version incompatible
//
//        // IO errors (500-599)
//        FileNotFound = 500,        ///< File doesn't exist
//        FileAccessDenied,          ///< Insufficient permissions
//        FileReadError,             ///< Error reading file
//        FileWriteError,            ///< Error writing file
//
//        // Memory errors (600-699)
//        OutOfMemory = 600,         ///< Memory allocation failed
//        InvalidAllocation,         ///< Invalid memory operation
//        MemoryCorruption,          ///< Memory corruption detected
//
//        // Threading errors (700-799)
//        DeadlockDetected = 700,    ///< Potential deadlock situation
//        InvalidThreadAccess,       ///< Thread-unsafe operation
//        SynchronizationError,      ///< Mutex/lock error
//
//        // Validation errors (800-899)
//        InvalidArgument = 800,     ///< Invalid function argument
//        InvalidState,              ///< Object in invalid state
//        OutOfRange,                ///< Value out of acceptable range
//
//        // Unknown errors (900-999)
//        UnknownError = 999         ///< Unspecified error
//    };
//
//    /**
//     * @brief Error context information for debugging
//     * 
//     * Captures source location and optional stack trace
//     * for detailed error diagnostics in debug builds.
//     */
//    struct ErrorContext
//    {
//        std::string file;                    ///< Source file where error occurred
//        int line;                            ///< Line number in source file
//        std::string function;                ///< Function name where error occurred
//        std::vector<std::string> stackTrace; ///< Optional stack trace (debug only)
//
//        /**
//         * @brief Construct error context from source location
//         * @param loc Source location (defaults to current location)
//         */
//        ErrorContext(const std::source_location& loc = std::source_location::current());
//    };
//
//    /**
//     * @brief Error class for detailed error reporting
//     * 
//     * Encapsulates error information including code, message,
//     * optional context, and nested errors. Thread-safe and
//     * immutable after construction.
//     */
//    class Error
//    {
//    public:
//        /**
//         * @brief Default constructor creates "no error" state
//         */
//        Error() = default;
//
//        /**
//         * @brief Construct error with code and message
//         * @param code Error code identifying the error type
//         * @param message Human-readable error description
//         */
//        Error(ErrorCode code, const std::string& message = "");
//
//        /**
//         * @brief Construct error with code, message, and source location
//         * @param code Error code identifying the error type
//         * @param message Human-readable error description
//         * @param loc Source location where error occurred
//         */
//        Error(ErrorCode code, const std::string& message,
//              const std::source_location& loc);
//
//        /**
//         * @brief Construct error with nested inner error
//         * @param code Error code identifying the error type
//         * @param message Human-readable error description
//         * @param inner Inner error that caused this error
//         */
//        Error(ErrorCode code, const std::string& message,
//              std::shared_ptr<Error> inner);
//
//        /**
//         * @brief Get the error code
//         * @return Error code enum value
//         */
//        ErrorCode code() const
//        {
//            return m_code;
//        }
//
//        /**
//         * @brief Get the error category based on code
//         * @return Category derived from error code
//         */
//        ErrorCategory category() const;
//
//        /**
//         * @brief Get the error message
//         * @return Human-readable error description
//         */
//        const std::string& message() const
//        {
//            return m_message;
//        }
//
//        /**
//         * @brief Get optional error context
//         * @return Context information if available
//         */
//        const std::optional<ErrorContext>& context() const
//        {
//            return m_context;
//        }
//
//        /**
//         * @brief Get nested inner error
//         * @return Inner error that caused this error, or nullptr
//         */
//        const std::shared_ptr<Error>& innerError() const
//        {
//            return m_innerError;
//        }
//
//        /**
//         * @brief Convert to string representation
//         * @return Basic error message with code
//         */
//        std::string toString() const;
//
//        /**
//         * @brief Convert to detailed string with full context
//         * @return Detailed error info including stack trace
//         */
//        std::string toDetailedString() const;
//
//        /**
//         * @brief Create a "no error" instance
//         * @return Error with ErrorCode::None
//         */
//        static Error none()
//        {
//            return Error();
//        }
//
//        /**
//         * @brief Check if this represents "no error"
//         * @return True if error code is None
//         */
//        bool isNone() const
//        {
//            return m_code == ErrorCode::None;
//        }
//
//    private:
//        ErrorCode m_code = ErrorCode::None;          ///< Error code identifier
//        std::string m_message;                       ///< Human-readable message
//        std::optional<ErrorContext> m_context;       ///< Optional debug context
//        std::shared_ptr<Error> m_innerError;         ///< Nested error (if any)
//    };
//
//    /**
//     * @brief Result type for fallible operations
//     * 
//     * Represents either a successful value of type T or an Error.
//     * Provides monadic operations for error handling without exceptions.
//     * 
//     * @tparam T Type of the successful result value
//     */
//    template<typename T>
//    class Result
//    {
//    public:
//        /**
//         * @brief Construct successful result with rvalue
//         * @param value Value to store (moved)
//         */
//        Result(T&& value) : m_value(std::move(value)), m_hasValue(true)
//        {
//        }
//
//        /**
//         * @brief Construct successful result with lvalue
//         * @param value Value to store (copied)
//         */
//        Result(const T& value) : m_value(value), m_hasValue(true)
//        {
//        }
//
//        /**
//         * @brief Construct error result with rvalue error
//         * @param error Error to store (moved)
//         */
//        Result(Error&& error) : m_error(std::move(error)), m_hasValue(false)
//        {
//        }
//
//        /**
//         * @brief Construct error result with lvalue error
//         * @param error Error to store (copied)
//         */
//        Result(const Error& error) : m_error(error), m_hasValue(false)
//        {
//        }
//
//        /**
//         * @brief Check if result contains a value
//         * @return True if successful (has value)
//         */
//        bool isOk() const
//        {
//            return m_hasValue;
//        }
//
//        /**
//         * @brief Check if result contains an error
//         * @return True if error occurred
//         */
//        bool isError() const
//        {
//            return !m_hasValue;
//        }
//
//        /**
//         * @brief Boolean conversion operator
//         * @return True if successful (has value)
//         */
//        explicit operator bool() const
//        {
//            return m_hasValue;
//        }
//
//        /**
//         * @brief Access the value (lvalue reference)
//         * @return Reference to contained value
//         * @throws std::runtime_error if result contains error
//         */
//        T& value()&
//        {
//            if (!m_hasValue) throw std::runtime_error(m_error.toString());
//            return m_value;
//        }
//
//        /**
//         * @brief Access the value (const lvalue reference)
//         * @return Const reference to contained value
//         * @throws std::runtime_error if result contains error
//         */
//        const T& value() const&
//        {
//            if (!m_hasValue) throw std::runtime_error(m_error.toString());
//            return m_value;
//        }
//
//        /**
//         * @brief Access the value (rvalue reference)
//         * @return Rvalue reference to contained value
//         * @throws std::runtime_error if result contains error
//         */
//        T&& value()&&
//        {
//            if (!m_hasValue) throw std::runtime_error(m_error.toString());
//            return std::move(m_value);
//        }
//
//        /**
//         * @brief Access the error
//         * @return Reference to contained error
//         * @throws std::runtime_error if result contains value
//         */
//        const Error& error() const
//        {
//            if (m_hasValue) throw std::runtime_error("No error in Result");
//            return m_error;
//        }
//
//        /**
//         * @brief Get value or default if error
//         * @param defaultValue Value to return if result contains error
//         * @return Contained value or default
//         */
//        T valueOr(const T& defaultValue) const
//        {
//            return m_hasValue ? m_value : defaultValue;
//        }
//
//        /**
//         * @brief Map the value to another type
//         * @tparam U Target type
//         * @tparam Func Function type (T -> U)
//         * @param func Mapping function
//         * @return Result<U> with mapped value or same error
//         */
//        template<typename U, typename Func>
//        Result<U> map(Func&& func) const
//        {
//            if (m_hasValue)
//            {
//                return Result<U>(func(m_value));
//            }
//            else
//            {
//                return Result<U>(m_error);
//            }
//        }
//
//        /**
//         * @brief Map the error to a new error
//         * @tparam Func Function type (Error -> Error)
//         * @param func Error mapping function
//         * @return Result<T> with same value or mapped error
//         */
//        template<typename Func>
//        Result<T> mapError(Func&& func) const
//        {
//            if (!m_hasValue)
//            {
//                return Result<T>(func(m_error));
//            }
//            else
//            {
//                return *this;
//            }
//        }
//
//        /**
//         * @brief Monadic bind operation
//         * @tparam U Target type
//         * @tparam Func Function type (T -> Result<U>)
//         * @param func Function that returns Result<U>
//         * @return Result<U> from function or propagated error
//         */
//        template<typename U, typename Func>
//        Result<U> andThen(Func&& func) const
//        {
//            if (m_hasValue)
//            {
//                return func(m_value);
//            }
//            else
//            {
//                return Result<U>(m_error);
//            }
//        }
//
//    private:
//        union
//        {
//            T m_value;      ///< Successful value (when m_hasValue is true)
//            Error m_error;  ///< Error information (when m_hasValue is false)
//        };
//        bool m_hasValue;    ///< Discriminant: true=value, false=error
//    };
//
//    /**
//     * @brief Specialization of Result for void type
//     * 
//     * Represents either success (no value) or an Error.
//     * Used for operations that don't return a value on success.
//     */
//    template<>
//    class Result<void>
//    {
//    public:
//        /**
//         * @brief Construct successful result
//         */
//        Result() : m_hasValue(true)
//        {
//        }
//
//        /**
//         * @brief Construct error result with rvalue error
//         * @param error Error to store (moved)
//         */
//        Result(Error&& error) : m_error(std::move(error)), m_hasValue(false)
//        {
//        }
//
//        /**
//         * @brief Construct error result with lvalue error
//         * @param error Error to store (copied)
//         */
//        Result(const Error& error) : m_error(error), m_hasValue(false)
//        {
//        }
//
//        /**
//         * @brief Check if operation succeeded
//         * @return True if successful
//         */
//        bool isOk() const
//        {
//            return m_hasValue;
//        }
//
//        /**
//         * @brief Check if operation failed
//         * @return True if error occurred
//         */
//        bool isError() const
//        {
//            return !m_hasValue;
//        }
//
//        /**
//         * @brief Boolean conversion operator
//         * @return True if successful
//         */
//        explicit operator bool() const
//        {
//            return m_hasValue;
//        }
//
//        /**
//         * @brief Access the error
//         * @return Reference to contained error
//         * @throws std::runtime_error if result is successful
//         */
//        const Error& error() const
//        {
//            if (m_hasValue) throw std::runtime_error("No error in Result");
//            return m_error;
//        }
//
//        /**
//         * @brief Create successful void result
//         * @return Result<void> representing success
//         */
//        static Result<void> ok()
//        {
//            return Result<void>();
//        }
//
//        /**
//         * @brief Create error void result
//         * @param error Error to return
//         * @return Result<void> containing error
//         */
//        static Result<void> err(Error&& error)
//        {
//            return Result<void>(std::move(error));
//        }
//
//    private:
//        Error m_error;      ///< Error information (when m_hasValue is false)
//        bool m_hasValue;    ///< Discriminant: true=success, false=error
//    };
//
//    /**
//     * @brief Helper function to create successful Result
//     * @tparam T Value type
//     * @param value Value to wrap in Result
//     * @return Result<T> containing the value
//     */
//    template<typename T>
//    Result<T> Ok(T&& value)
//    {
//        return Result<T>(std::forward<T>(value));
//    }
//
//    /**
//     * @brief Helper function to create error Result
//     * @tparam T Result value type
//     * @param error Error to wrap in Result
//     * @return Result<T> containing the error
//     */
//    template<typename T>
//    Result<T> Err(Error&& error)
//    {
//        return Result<T>(std::move(error));
//    }
//
//    /**
//     * @brief Helper function to create successful void Result
//     * @return Result<void> representing success
//     */
//    inline Result<void> Ok()
//    {
//        return Result<void>();
//    }
//
//    /**
//     * @brief Helper function to create error void Result
//     * @param error Error to wrap in Result
//     * @return Result<void> containing the error
//     */
//    inline Result<void> Err(Error&& error)
//    {
//        return Result<void>(std::move(error));
//    }
//
//    /**
//     * @brief Macro for creating errors with source location
//     * 
//     * Automatically captures file, line, and function information
//     * for error context in debug builds.
//     * 
//     * @param code ErrorCode value
//     * @param message Error message string
//     */
//#define MAKE_ERROR(code, message) \
//        Error(code, message, std::source_location::current())
//
//    /**
//     * @brief Macro for propagating errors from Result expressions
//     * 
//     * Evaluates expression and returns early with error if Result is not ok.
//     * Use for void-returning functions.
//     * 
//     * @param expr Expression returning Result<T>
//     */
//#define TRY(expr) \
//        do { \
//            auto _result = (expr); \
//            if (!_result.isOk()) return Err(_result.error()); \
//        } while(0)
//
//    /**
//     * @brief Macro for extracting values from Result with error propagation
//     * 
//     * Evaluates expression, extracts value if ok, or returns early with error.
//     * Creates a variable with the extracted value.
//     * 
//     * @param var Variable to assign the extracted value
//     * @param expr Expression returning Result<T>
//     */
//#define TRY_ASSIGN(var, expr) \
//        auto _result_##var = (expr); \
//        if (!_result_##var.isOk()) return Err(_result_##var.error()); \
//        var = std::move(_result_##var.value());
//
//} // namespace PixelCraft::ECS