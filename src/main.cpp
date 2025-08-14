// -------------------------------------------------------------------------
// main.cpp
// -------------------------------------------------------------------------

#include "Core/Application.h"
#include "Core/Logger.h"
#include "Debug/UnitTestFramework.h"
#include "Debug/TextureTestSuite.h"
#include "Debug/ShaderTestSuite.h"
#include "Debug/MaterialTestSuite.h"
#include "Rendering/RenderSystem.h"

#include <algorithm>
#include <exception>
#include <iostream>
#include <string>
#include <vector>

using namespace PixelCraft;

/**
 * @brief Command line arguments structure
 */
struct CommandLineArgs
{
    bool runTests = false;
    bool runQuickTests = false;
    bool runFullTestSuite = false;
    bool showHelp = false;
    bool verboseOutput = false;
    std::vector<std::string> testCategories;
    std::vector<std::string> specificTests;
    std::string outputFile = "test_results.xml";
    std::string logLevel = "info";
};

/**
 * @brief Parse command line arguments
 * @param argc Argument count
 * @param argv Argument values
 * @return Parsed command line arguments
 */
CommandLineArgs parseCommandLine(int argc, char *argv[])
{
    CommandLineArgs args;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg(argv[i]);

        if (arg == "--test" || arg == "-t")
        {
            args.runTests = true;
        }
        else if (arg == "--test-quick" || arg == "-tq")
        {
            args.runTests = true;
            args.runQuickTests = true;
        }
        else if (arg == "--test-full" || arg == "-tf")
        {
            args.runTests = true;
            args.runFullTestSuite = true;
        }
        else if (arg == "--help" || arg == "-h")
        {
            args.showHelp = true;
        }
        else if (arg == "--verbose" || arg == "-v")
        {
            args.verboseOutput = true;
        }
        else if (arg == "--category" || arg == "-c")
        {
            if (i + 1 < argc)
            {
                args.testCategories.push_back(argv[++i]);
                args.runTests = true;
            }
        }
        else if (arg == "--test-name" || arg == "-tn")
        {
            if (i + 1 < argc)
            {
                args.specificTests.push_back(argv[++i]);
                args.runTests = true;
            }
        }
        else if (arg == "--output" || arg == "-o")
        {
            if (i + 1 < argc)
            {
                args.outputFile = argv[++i];
            }
        }
        else if (arg == "--log-level" || arg == "-l")
        {
            if (i + 1 < argc)
            {
                args.logLevel = argv[++i];
            }
        }
        else if (arg.substr(0, 2) == "--")
        {
            std::cerr << "Unknown option: " << arg << std::endl;
        }
    }

    return args;
}

/**
 * @brief Display help information
 */
void showHelp()
{
    std::cout << "PixelCraft Engine - Usage Options\n";
    std::cout << "==================================\n\n";

    std::cout << "Normal Application:\n";
    std::cout << "  <no arguments>                 Run the normal application\n\n";

    std::cout << "Unit Testing:\n";
    std::cout << "  --test, -t                     Run default test suite\n";
    std::cout << "  --test-quick, -tq              Run essential tests only (CI quick feedback)\n";
    std::cout << "  --test-full, -tf               Run comprehensive test suite\n";
    std::cout << "  --category <name>, -c <name>   Run tests in specific category\n";
    std::cout << "  --test-name <name>, -tn <name> Run a specific named test\n\n";

    std::cout << "Available Test Categories:\n";
    std::cout << "  Global:\n";
    std::cout << "    Basic                        Core functionality tests\n";
    std::cout << "    OpenGL                       OpenGL integration tests\n";
    std::cout << "    Performance                  Performance and memory tests\n";
    std::cout << "    ErrorHandling                Error handling and edge cases\n";
    std::cout << "    EdgeCases                    Boundary condition tests\n";
    std::cout << "    Integration                  Complex workflow tests\n\n";
    std::cout << "  Texture:\n";
    std::cout << "    Formats                      Texture format and data tests\n";
    std::cout << "    FileIO                       File loading and saving tests\n";
    std::cout << "    Pixels                       Pixel manipulation tests\n";
    std::cout << "    Resizing                     Texture resizing and format conversion\n";
    std::cout << "    Filtering                    Filter and wrap mode tests\n";
    std::cout << "    Multisampling                Multisampling functionality tests\n";
    std::cout << "    Mipmaps                      Mipmap generation and usage tests\n";
    std::cout << "    PixelArt                     Pixel art specific features\n\n";
    std::cout << "  Shader:\n";
    std::cout << "    SourceManagement             Shader source management tests\n";
    std::cout << "    Compilation                  Shader compilation tests\n";
    std::cout << "    Uniforms                     Shader uniform getting and setting tests\n";
    std::cout << "    Attributes                   Shader attribute getting and setting tests\n";
    std::cout << "    Compute                      Compute shader tests\n";
    std::cout << "    HotReload                    Shader hot reloading tests\n\n";
    std::cout << "  Material:\n";
    std::cout << "    Shader                       Material shader management and binding tests\n";
    std::cout << "    Parameters                   Material parameter management tests\n";
    std::cout << "    Textures                     Material texture management (slots and custom) tests\n";
    std::cout << "    PBR                          Material PBR properties tests\n";
    std::cout << "    RenderState                  Material render state management\n";
    std::cout << "    Inheritance                  Material inheritance and parenting tests\n";
    std::cout << "    Cloning                      Material cloning and comparison operator tests\n";
    std::cout << "    Binding                      Material binding and OpenGL integration tests\n\n";
    
    std::cout << "Output Options:\n";
    std::cout << "  --output <file>, -o <file>     XML output file for CI (default: test_results.xml)\n";
    std::cout << "  --verbose, -v                  Verbose test output\n";
    std::cout << "  --log-level <level>, -l <level> Set log level (debug, info, warn, error, fatal)\n\n";

    std::cout << "Examples:\n";
    std::cout << "  PixelCraftEngine                          # Run normal application\n";
    std::cout << "  PixelCraftEngine --test                   # Run default test suite\n";
    std::cout << "  PixelCraftEngine --test-quick             # Run quick tests for CI\n";
    std::cout << "  PixelCraftEngine --test-full              # Run comprehensive tests\n";
    std::cout << "  PixelCraftEngine -c Basic -c OpenGL       # Run specific categories\n";
    std::cout << "  PixelCraftEngine -tn testConstruction     # Run specific test\n";
    std::cout << "  PixelCraftEngine --test -o results.xml    # Run tests with custom output\n";
    std::cout << "  PixelCraftEngine --test --verbose         # Run tests with verbose output\n";
    std::cout << std::endl;
}

/**
 * @brief Set up logging based on command line arguments
 * @param args Command line arguments
 */
void setupLogging(const CommandLineArgs &args)
{
    auto &logger = Core::Logger::getInstance();

    // Set log level based on arguments
    if (args.logLevel == "debug")
    {
        logger.setLogLevel(Core::LogLevel::Debug);
    }
    else if (args.logLevel == "info")
    {
        logger.setLogLevel(Core::LogLevel::Info);
    }
    else if (args.logLevel == "warn")
    {
        logger.setLogLevel(Core::LogLevel::Warning);
    }
    else if (args.logLevel == "error")
    {
        logger.setLogLevel(Core::LogLevel::Error);
    }
    else if (args.logLevel == "fatal")
    {
        logger.setLogLevel(Core::LogLevel::Fatal);
    }
}

/**
 * @brief Run tests using the accumulative test session approach
 * @param testFramework The test framework to use
 * @param categories List of categories to run
 */
void runTestCategoriesAccumulative(Debug::RenderingTestFramework& testFramework, const std::vector<std::string>& categories)
{
    Core::info("Starting accumulative test session for " + std::to_string(categories.size()) + " categories...");

    // Start the test session
    testFramework.startTestSession();

    // Run each category, accumulating results
    for (const auto& category : categories)
    {
        Core::info("Running category: " + category);
        testFramework.runTestsInCategoryAccumulative(category);
    }

    // End the test session
    testFramework.endTestSession();

    Core::info("Accumulative test session completed successfully");
}

/**
 * @brief Run the unit test suite
 * @param args Command line arguments
 * @return Exit code (0 for success, non-zero for failure)
 */
int runUnitTests(const CommandLineArgs &args)
{
    Core::info("Starting PixelCraft Engine Unit Tests");

    try
    {
        // =============================================================================
        // Setup Minimal Application for Testing
        // =============================================================================

        Core::info("Initializing minimal application for testing...");

        // Get application instance
        auto app = Core::Application::getInstance();
        if (!app)
        {
            Core::error("Failed to create application instance for testing");
            return -1;
        }

        // Initialize the application in minimal/headless mode if possible
        if (!app->initialize())
        {
            Core::error("Failed to initialize application for testing");
            return -1;
        }

        // Get render system from application
        auto renderSystem = app->getSubsystem<Rendering::RenderSystem>();
        if (!renderSystem)
        {
            Core::error("Failed to get render system for testing");
            app->shutdown();
            return -1;
        }

        Core::info("Test environment initialized successfully");

        // =============================================================================
        // Setup Testing Framework
        // =============================================================================

        Core::info("Setting up rendering test framework...");

        // Create rendering test framework (specialized for OpenGL tests)
        Debug::RenderingTestFramework testFramework;

        // Configure the framework
        testFramework.setApplication(app);
        testFramework.setRenderSystem(renderSystem);
        testFramework.enableOpenGLContext(true);

        // =============================================================================
        // Register Test Suites
        // =============================================================================

        Core::info("Registering test suites...");

        // Create and register texture test suite
        Debug::TextureTestSuite textureTestSuite(testFramework);
        textureTestSuite.registerTests();

        // Create and register shader test suite
        Debug::ShaderTestSuite shaderTestSuite(testFramework);
        shaderTestSuite.registerTests();

        // Create and register material test suite
        Debug::MaterialTestSuite materialTestSuite(testFramework);
        materialTestSuite.registerTests();

        Core::info("Test suites registered successfully");

        // =============================================================================
        // Run Tests Based on Arguments
        // =============================================================================

        if (args.runQuickTests)
        {
            Core::info("Running quick test suite for CI...");
            std::vector<std::string> quickCategories = {"Basic", "OpenGL", "Formats", "SourceManagement"};
            runTestCategoriesAccumulative(testFramework, quickCategories);
        }
        else if (args.runFullTestSuite)
        {
            Core::info("Running comprehensive test suite...");
            testFramework.runAllTests();
        }
        else if (!args.testCategories.empty())
        {
            Core::info("Running specific test categories...");
            runTestCategoriesAccumulative(testFramework, args.testCategories);
        }
        else if (!args.specificTests.empty())
        {
            Core::info("Running specific tests...");
            // For specific tests, we need to run them individually
            // Start a test session for accumulation
            testFramework.startTestSession();
            
            for (const auto &testName : args.specificTests)
            {
                Core::info("Running test: " + testName);
                testFramework.runSingleTest(testName);
            }
            
            testFramework.endTestSession();
        }
        else
        {
            // Default test run - core functionality using accumulative approach
            Core::info("Running default test suite...");
            std::vector<std::string> defaultCategories = {
                "Basic", "OpenGL", "Formats", "Pixels", "Filtering",
                "SourceManagement", "Compilation", "Uniforms", "Attributes"
            };
            runTestCategoriesAccumulative(testFramework, defaultCategories);
        }

        // =============================================================================
        // Generate Reports and Cleanup
        // =============================================================================

        Core::info("Generating test reports...");

        // Print results to console
        if (args.verboseOutput)
        {
            testFramework.printDetailedResults();
        }
        else
        {
            testFramework.printResults();
        }

        // Export results to XML for CI integration
        testFramework.exportResultsToFile(args.outputFile);
        Core::info("Test results exported to: " + args.outputFile);

        // Get final statistics
        auto stats = testFramework.getStatistics();

        Core::info("Test execution complete");
        Core::info("Total Tests: " + std::to_string(stats.totalTests));
        Core::info("Passed: " + std::to_string(stats.passedTests));
        Core::info("Failed: " + std::to_string(stats.failedTests));
        Core::info("Pass Rate: " + std::to_string(static_cast<int>(stats.getPassRate())) + "%");

        // Always print comprehensive results at the end for better visibility
        if (!args.verboseOutput)
        {
            std::cout << "\n" << std::string(80, '=') << std::endl;
            std::cout << "COMPREHENSIVE TEST RESULTS" << std::endl;
            std::cout << std::string(80, '=') << std::endl;
            
            // Group results by category for better organization
            std::map<std::string, std::vector<Debug::TestExecutionResult>> resultsByCategory;
            for (const auto &result : testFramework.getResults())
            {
                resultsByCategory[result.category].push_back(result);
            }
            
            // Print results organized by category
            for (const auto &[category, categoryResults] : resultsByCategory)
            {
                std::cout << "\nCategory: " << category << " (" << categoryResults.size() << " tests)" << std::endl;
                std::cout << std::string(40, '-') << std::endl;
                
                for (const auto &result : categoryResults)
                {
                    std::cout << "  " << std::left << std::setw(35) << result.testName;
                    std::cout << std::setw(10) << (result.result == Debug::TestResult::PASSED ? "PASSED" :
                                                   result.result == Debug::TestResult::FAILED ? "FAILED" :
                                                   result.result == Debug::TestResult::SKIPPED ? "SKIPPED" : "ERRORED");
                    std::cout << " (" << result.executionTime.count() << "ms)" << std::endl;
                    
                    if (!result.errorMessage.empty() && result.result != Debug::TestResult::PASSED)
                    {
                        std::cout << "    Error: " << result.errorMessage << std::endl;
                    }
                }
            }
            
            std::cout << std::string(80, '=') << std::endl;
        }

        // Clean up application
        app->shutdown();

        // Return appropriate exit code for CI systems
        if (stats.failedTests == 0 && stats.erroredTests == 0)
        {
            Core::info("All tests passed successfully!");
            return 0;
        }
        else
        {
            Core::error("Some tests failed - see output for details");
            return 1;
        }
    }
    catch (const std::exception &e)
    {
        Core::fatal("Exception during unit testing: " + std::string(e.what()));
        return -1;
    }
    catch (...)
    {
        Core::fatal("Unknown exception during unit testing");
        return -1;
    }
}

/**
 * @brief Run the normal application
 * @param args Command line arguments
 * @return Exit code (0 for success, non-zero for failure)
 */
int runNormalApplication(const CommandLineArgs &args)
{
    (void)args;  // Not currently used
    Core::info("Starting PixelCraft Engine");

    try
    {
        // Get application instance
        auto app = Core::Application::getInstance();
        if (!app)
        {
            Core::error("Failed to create application instance");
            return -1;
        }

        // Initialize the application
        if (!app->initialize())
        {
            Core::error("Failed to initialize application");
            return -1;
        }

        Core::info("Application initialized successfully");

        // Run the main loop
        app->run();

        // Shutdown
        app->shutdown();

        Core::info("Application shutdown complete");
        return 0;
    }
    catch (const std::exception &e)
    {
        Core::fatal("Exception in application: " + std::string(e.what()));
        return -1;
    }
    catch (...)
    {
        Core::fatal("Unknown exception in application");
        return -1;
    }
}

/**
 * @brief Main entry point
 * @param argc Argument count
 * @param argv Argument values
 * @return Exit code
 */
int main(int argc, char *argv[])
{
    // Parse command line arguments
    CommandLineArgs args = parseCommandLine(argc, argv);

    // Show help if requested
    if (args.showHelp)
    {
        showHelp();
        return 0;
    }

    // Initialize logger first with rotation support
    Core::LogRotationConfig rotationConfig;
    rotationConfig.logDirectory = "logs";
    rotationConfig.logPrefix = "pixelcraft";
    rotationConfig.latestLogFilename = "latest.txt";
    rotationConfig.maxLogFiles = 10;
    rotationConfig.enableTimestamps = true;
    rotationConfig.enableLatestSymlink = true;

    // Initialize logger first
    if (!Core::Logger::getInstance().initializeWithRotation(rotationConfig))
    {
        std::cerr << "Failed to initialize logger!" << std::endl;
        return -1;
    }

    // Setup logging based on arguments (adds additional channels if needed)
    //setupLogging(args);
    auto &logger = Core::Logger::getInstance();
    logger.setLogLevel(Core::LogLevel::Debug); // Default to debug for now

    Core::info("PixelCraft Engine starting up...");
    Core::info("Log rotation enabled - logs saved to: " + rotationConfig.logDirectory);

    int exitCode = 0;

    try
    {
        // Determine execution mode
        if (args.runTests)
        {
            exitCode = runUnitTests(args);
        }
        else
        {
            exitCode = runNormalApplication(args);
        }
    }
    catch (const std::exception &e)
    {
        Core::fatal("Fatal error: " + std::string(e.what()));
        exitCode = -1;
    }
    catch (...)
    {
        Core::fatal("Unknown fatal error occurred");
        exitCode = -1;
    }

    Core::info("PixelCraft Engine shutting down with exit code: " + std::to_string(exitCode));

    // Always shutdown logger before exit
    Core::Logger::getInstance().shutdown();

    return exitCode;
}