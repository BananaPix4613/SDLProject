//// -------------------------------------------------------------------------
//// Error.cpp
//// -------------------------------------------------------------------------
//#include "ECS/Error.h"
//
//#include <sstream>
//#include <iomanip>
//
//namespace PixelCraft::ECS
//{
//    ErrorContext::ErrorContext(const std::source_location& loc)
//        : file(loc.file_name())
//        , line(loc.line())
//        , function(loc.function_name())
//    {
//        // In a real implementation, we'd capture stack trace here
//        // This would typically use platform-specific APIs:
//        // - Windows: CaptureStackBackTrace + SymFromAddr
//        // - POSIX: backtrace + backtrace_symbols
//
//#ifdef PIXELCRAFT_DEBUG
//// Simplified pseudo-implementation for stack trace:
//// stackTrace = captureStackTrace();
//#endif
//    }
//
//    Error::Error(ErrorCode code, const std::string& message)
//        : m_code(code)
//        , m_message(message)
//        , m_context(std::nullopt)
//        , m_innerError(nullptr)
//    {
//    }
//
//    Error::Error(ErrorCode code, const std::string& message, const std::source_location& loc)
//        : m_code(code)
//        , m_message(message)
//        , m_context(ErrorContext(loc))
//        , m_innerError(nullptr)
//    {
//    }
//
//    Error::Error(ErrorCode code, const std::string& message, std::shared_ptr<Error> inner)
//        : m_code(code)
//        , m_message(message)
//        , m_context(std::nullopt)
//        , m_innerError(inner)
//    {
//    }
//
//    ErrorCategory Error::category() const
//    {
//        // Determine category based on error code range
//        int codeValue = static_cast<int>(m_code);
//
//        if (m_code == ErrorCode::None)
//        {
//            return ErrorCategory::None;
//        }
//        else if (codeValue >= 100 && codeValue < 200)
//        {
//            return ErrorCategory::Entity;
//        }
//        else if (codeValue >= 200 && codeValue < 300)
//        {
//            return ErrorCategory::Component;
//        }
//        else if (codeValue >= 300 && codeValue < 400)
//        {
//            return ErrorCategory::System;
//        }
//        else if (codeValue >= 400 && codeValue < 500)
//        {
//            return ErrorCategory::Serialization;
//        }
//        else if (codeValue >= 500 && codeValue < 600)
//        {
//            return ErrorCategory::IO;
//        }
//        else if (codeValue >= 600 && codeValue < 700)
//        {
//            return ErrorCategory::Memory;
//        }
//        else if (codeValue >= 700 && codeValue < 800)
//        {
//            return ErrorCategory::Threading;
//        }
//        else if (codeValue >= 800 && codeValue < 900)
//        {
//            return ErrorCategory::Validation;
//        }
//
//        return ErrorCategory::Unknown;
//    }
//
//    std::string Error::toString() const
//    {
//        if (isNone())
//        {
//            return "No error";
//        }
//
//        std::stringstream ss;
//
//        // Format: [Category.ErrorCode] Message
//        ss << "[" << static_cast<int>(category()) << "."
//            << static_cast<int>(m_code) << "] " << m_message;
//
//        // Add inner error reference if present
//        if (m_innerError)
//        {
//            ss << " (Caused by: " << m_innerError->toString() << ")";
//        }
//
//        return ss.str();
//    }
//
//    std::string Error::toDetailedString() const
//    {
//        if (isNone())
//        {
//            return "No error";
//        }
//
//        std::stringstream ss;
//
//        // Basic error information
//        ss << "Error Code: " << static_cast<int>(m_code) << std::endl;
//        ss << "Category: " << static_cast<int>(category()) << std::endl;
//        ss << "Message: " << m_message << std::endl;
//
//        // Add context information if available
//        if (m_context)
//        {
//            ss << "Location: " << m_context->file << ":" << m_context->line
//                << " in " << m_context->function << std::endl;
//
//            // Add stack trace if available
//            if (!m_context->stackTrace.empty())
//            {
//                ss << "Stack Trace:" << std::endl;
//                for (size_t i = 0; i < m_context->stackTrace.size(); i++)
//                {
//                    ss << "  " << std::setw(2) << i << ": "
//                        << m_context->stackTrace[i] << std::endl;
//                }
//            }
//        }
//
//        // Add inner error details if present
//        if (m_innerError)
//        {
//            ss << "Caused by:" << std::endl;
//            ss << "-------------------" << std::endl;
//            ss << m_innerError->toDetailedString();
//        }
//
//        return ss.str();
//    }
//
//} // namespace PixelCraft::ECS