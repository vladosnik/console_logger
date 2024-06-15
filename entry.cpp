#include <Windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <thread>
#include <fstream> // for file reading
#include <codecvt>  // For std::wstring_convert and std::codecvt_utf8

#include <ctime>    // for std::time
#include <iomanip>  // for std::put_time

#include <mutex>    // For std::mutex and std::lock_guard




#define FOREGROUND_WHITE (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)
#define FOREGROUND_LIGHT_RED (FOREGROUND_RED | FOREGROUND_INTENSITY)
#define FOREGROUND_LIGHT_GREEN (FOREGROUND_GREEN | FOREGROUND_INTENSITY)
#define FOREGROUND_CYAN (FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY)

HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);  // Get handle to console output

std::mutex consoleMutex; // Mutex for synchronizing console output



// Function to simulate file open operation every second
void SimulateFileOpen(const std::wstring& fullPath)
{
    while (true)
    {
        HANDLE hFile = CreateFileW(
            fullPath.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );

        if (hFile != INVALID_HANDLE_VALUE)
        {
            CloseHandle(hFile);
        }
        else
        {
            std::lock_guard<std::mutex> lock(consoleMutex);
            SetConsoleTextAttribute(hConsole, FOREGROUND_LIGHT_RED);
            std::wcout << L"Error opening file for simulation: " << fullPath << L", Error code: " << GetLastError() << std::endl;
            SetConsoleTextAttribute(hConsole, FOREGROUND_WHITE);
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

// Function to open a directory for monitoring
HANDLE OpenDirectory(const std::wstring& directory)
{
    HANDLE hDirectory = CreateFileW(
        directory.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        NULL
    );

    if (hDirectory == INVALID_HANDLE_VALUE)
    {
        std::lock_guard<std::mutex> lock(consoleMutex);
        SetConsoleTextAttribute(hConsole, FOREGROUND_LIGHT_RED);
        std::wcout << L"Error opening directory: " << directory << L", Error code: " << GetLastError() << std::endl;
        SetConsoleTextAttribute(hConsole, FOREGROUND_WHITE);
    }
    else
    {
        std::lock_guard<std::mutex> lock(consoleMutex);
        SetConsoleTextAttribute(hConsole, FOREGROUND_LIGHT_GREEN);
        std::wcout << std::endl << L"Successfully opened directory for monitoring: " << directory << std::endl;
        SetConsoleTextAttribute(hConsole, FOREGROUND_WHITE);

    }

    return hDirectory;
}

// Function to open a file for monitoring
HANDLE OpenFileForMonitoring(const std::wstring& fullPath)
{
    HANDLE hFile = CreateFileW(
        fullPath.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE)
    {
        std::lock_guard<std::mutex> lock(consoleMutex);
        SetConsoleTextAttribute(hConsole, FOREGROUND_LIGHT_RED);
        std::wcout << L"Error opening file: " << fullPath << L", Error code: " << GetLastError() << std::endl;
        SetConsoleTextAttribute(hConsole, FOREGROUND_WHITE);

    }
    else
    {
        std::lock_guard<std::mutex> lock(consoleMutex);
        SetConsoleTextAttribute(hConsole, FOREGROUND_LIGHT_GREEN);
        std::wcout << std::endl << L"Successfully opened file for monitoring: " << fullPath << std::endl;
        SetConsoleTextAttribute(hConsole, FOREGROUND_WHITE);

    }

    return hFile;
}

// Function to get the size of a file
bool GetFileSize(HANDLE hFile, LARGE_INTEGER& fileSize)
{
    if (!GetFileSizeEx(hFile, &fileSize))
    {
        std::lock_guard<std::mutex> lock(consoleMutex);
        SetConsoleTextAttribute(hConsole, FOREGROUND_LIGHT_RED);
        std::wcout << L"Error getting file size: " << GetLastError() << std::endl;
        SetConsoleTextAttribute(hConsole, FOREGROUND_WHITE);

        return false;
    }
    return true;
}

// Function to read newly added content in a file
bool ReadNewContent(HANDLE hFile, LARGE_INTEGER currentFileSize)
{
    LARGE_INTEGER fileSize;
    if (!GetFileSize(hFile, fileSize))
    {
        return false;
    }

    // Calculate the size of newly added content
    LARGE_INTEGER newContentSize;
    newContentSize.QuadPart = fileSize.QuadPart - currentFileSize.QuadPart;
    if (newContentSize.QuadPart > 0)
    {
        LARGE_INTEGER distanceToMove;
        distanceToMove.QuadPart = currentFileSize.QuadPart;
        SetFilePointerEx(hFile, distanceToMove, NULL, FILE_BEGIN);

        DWORD newBytesToRead = static_cast<DWORD>(newContentSize.QuadPart);
        std::vector<char> buffer(newBytesToRead + 1, 0); // +1 for null terminator
        DWORD bytesRead;

        if (ReadFile(hFile, &buffer[0], newBytesToRead, &bytesRead, NULL))
        {
            
            // Get current time
            std::time_t now = std::time(nullptr);
            struct tm timeinfo;
            localtime_s(&timeinfo, &now);

            // Format timestamp
            wchar_t timestamp[20];
            wcsftime(timestamp, sizeof(timestamp), L"%Y-%m-%d %H:%M:%S", &timeinfo);

            // Output timestamp and content
            std::lock_guard<std::mutex> lock(consoleMutex);
            SetConsoleTextAttribute(hConsole, FOREGROUND_CYAN);
            std::wcout << L"[" << timestamp << L"] Newly added content:" << std::endl;
            SetConsoleTextAttribute(hConsole, FOREGROUND_WHITE);
            std::cout.write(buffer.data(), bytesRead);
            std::cout << std::endl;
        }
        else
        {
            std::lock_guard<std::mutex> lock(consoleMutex);
            SetConsoleTextAttribute(hConsole, FOREGROUND_LIGHT_RED);
            std::wcout << L"Error reading file: " << GetLastError() << std::endl;
            SetConsoleTextAttribute(hConsole, FOREGROUND_WHITE);

            return false;
        }
    }

    return true;
}

// Function to monitor file changes for a single file
void MonitorFileChanges(const std::wstring& directory, const std::wstring& filename)
{
    std::wstring fullPath = directory + L"\\" + filename;

    HANDLE hDirectory = OpenDirectory(directory);
    if (hDirectory == INVALID_HANDLE_VALUE)
    {
        return;
    }

    HANDLE hFile = OpenFileForMonitoring(fullPath);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        CloseHandle(hDirectory);
        return;
    }

    // Get initial file size
    LARGE_INTEGER initialFileSize;
    if (!GetFileSize(hFile, initialFileSize))
    {
        CloseHandle(hFile);
        CloseHandle(hDirectory);
        return;
    }

    // Start monitoring from the current file size
    LARGE_INTEGER currentFileSize = initialFileSize;

    BYTE buffer[4096];
    DWORD bytesReturned;

    OVERLAPPED overlapped = { 0 };
    overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (overlapped.hEvent == NULL)
    {
        std::lock_guard<std::mutex> lock(consoleMutex);
        SetConsoleTextAttribute(hConsole, FOREGROUND_LIGHT_RED);
        std::wcout << L"Error creating event: " << GetLastError() << std::endl;
        SetConsoleTextAttribute(hConsole, FOREGROUND_WHITE);

        CloseHandle(hDirectory);
        return;
    }

    while (true)
    {
        if (!ReadDirectoryChangesW(
            hDirectory,
            buffer,
            sizeof(buffer),
            TRUE, // watch subtree
            FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_SIZE,
            &bytesReturned,
            &overlapped,
            NULL))
        {
            DWORD error = GetLastError();
            if (error != ERROR_IO_PENDING)
            {
                std::lock_guard<std::mutex> lock(consoleMutex);
                SetConsoleTextAttribute(hConsole, FOREGROUND_LIGHT_RED);
                std::wcout << L"Error reading directory changes: " << error << std::endl;
                SetConsoleTextAttribute(hConsole, FOREGROUND_WHITE);

                break;
            }
        }

        // Wait for changes
        DWORD result = WaitForSingleObject(overlapped.hEvent, INFINITE);
        if (result == WAIT_OBJECT_0)
        {
            DWORD bytesTransferred;
            if (!GetOverlappedResult(hDirectory, &overlapped, &bytesTransferred, FALSE))
            {
                std::lock_guard<std::mutex> lock(consoleMutex);
                SetConsoleTextAttribute(hConsole, FOREGROUND_LIGHT_RED);
                std::wcout << L"Error getting overlapped result: " << GetLastError() << std::endl;
                SetConsoleTextAttribute(hConsole, FOREGROUND_WHITE);

                break;
            }

            // Re-open file to read the newly added content
            hFile = OpenFileForMonitoring(fullPath);
            if (hFile != INVALID_HANDLE_VALUE)
            {
                if (ReadNewContent(hFile, currentFileSize))
                {
                    GetFileSize(hFile, currentFileSize);
                }

                CloseHandle(hFile);
            }
            else
            {
                std::lock_guard<std::mutex> lock(consoleMutex);
                SetConsoleTextAttribute(hConsole, FOREGROUND_LIGHT_RED);
                std::wcout << L"Error opening file: " << fullPath << L", Error code: " << GetLastError() << std::endl;
                SetConsoleTextAttribute(hConsole, FOREGROUND_WHITE);

            }

            // Reset event for next notification
            ResetEvent(overlapped.hEvent);
        }
        else
        {
            std::lock_guard<std::mutex> lock(consoleMutex);
            SetConsoleTextAttribute(hConsole, FOREGROUND_LIGHT_RED);
            std::wcout << L"WaitForSingleObject failed: " << GetLastError() << std::endl;
            SetConsoleTextAttribute(hConsole, FOREGROUND_WHITE);

            break;
        }
    }

    CloseHandle(overlapped.hEvent);
    CloseHandle(hDirectory);
}


// Function to monitor file changes for multiple files in different directories
void MonitorMultipleFiles(const std::vector<std::pair<std::wstring, std::wstring>>& files)
{
    std::vector<std::thread> threads;

    for (const auto& entry : files)
    {
        std::wstring directory = entry.first;
        std::wstring filename = entry.second;

        threads.emplace_back(MonitorFileChanges, directory, filename);
    }

    for (auto& thread : threads)
    {
        thread.join();
    }
}





std::wstring to_wstring(const std::string& stringToConvert) {

    std::wstring wideString =

        std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(stringToConvert);

    return wideString;

}


std::vector<std::pair<std::wstring, std::wstring>> readFilesList(const std::wstring& filename) {

    std::vector<std::pair<std::wstring, std::wstring>> filesToMonitor;

    std::wifstream file(filename);

    std::wstring line;


    while (std::getline(file, line)) {

        size_t lastSlash = line.find_last_of(L"\\");

        if (lastSlash != std::wstring::npos) {

            std::wstring directory = line.substr(0, lastSlash);

            std::wstring file = line.substr(lastSlash + 1);

            filesToMonitor.push_back({ directory, file });

        }

    }


    return filesToMonitor;

}


int main(int argc, char* argv[])
{

    std::wstring filename = L"C:\\Users\\rocke\\source\\repos\\console_logger\\x64\\Debug\\files_list.txt";  // Replace with your file path

    std::wcout << L"default filename: " << filename << std::endl;

    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <files_list_path>" << std::endl;
    }
    else {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        filename = converter.from_bytes(argv[1]);
        std::wcout << L"specified filename: " << filename << std::endl;
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));





    std::vector<std::pair<std::wstring, std::wstring>> filesToMonitor = readFilesList(filename);

    // Output the formatted C++ vector
    std::wcout << std::endl << L"std::vector<std::pair<std::wstring, std::wstring>> filesToMonitor = {" << std::endl;
    for (const auto& entry : filesToMonitor)
    {
        std::wcout << L"    { L\"" << entry.first << L"\", L\"" << entry.second << L"\" }," << std::endl;
    }
    std::wcout << L"};" << std::endl << std::endl;

    // Start the file open simulation in a separate thread for each file
    std::vector<std::thread> simulateFileOpenThreads;
    for (const auto& entry : filesToMonitor)
    {
        std::wstring directory = entry.first;
        std::wstring filename = entry.second;
        std::wstring fullPath = directory + L"\\" + filename;

        simulateFileOpenThreads.emplace_back(SimulateFileOpen, fullPath);
    }

    // Start monitoring changes for all files
    MonitorMultipleFiles(filesToMonitor);

    // Wait for simulation threads to complete
    for (auto& thread : simulateFileOpenThreads)
    {
        thread.join();
    }

    return 0;
}
