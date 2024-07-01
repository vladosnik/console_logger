console_logger.exe is command line tool, to listen changes on text files and log new contents.

compiled on Windows 11 with visual studio

# Example usage:
1) download console_logger.exe
2) create .bat file in same folder with console_logger.exe, that will run "console_logger.exe <path/to/files_list.txt>" as on the example

#example contents of "run console_logger.bat"
```
@echo off
console_logger.exe D:\custom_programs\console_logger\files_list.txt
```
3) create shortcut to this .bat file and run it as administrator

# Example of text file that contains files list parsed by console_logger

files_list.txt example:
```
D:\Vlados\Projects\web\vsystem\domains\store\logs\store.access.log
C:\Program Files\FileZilla Server\Logs\filezilla-server.log
C:\Program Files\FileZilla Server\Logs\filezilla-server.1.log
C:\Program Files\FileZilla Server\Logs\filezilla-server.2.log
C:\Program Files\FileZilla Server\Logs\filezilla-server.3.log
C:\Program Files\FileZilla Server\Logs\filezilla-server.4.log
C:\Program Files\FileZilla Server\Logs\filezilla-server.5.log
```
just write paths to text files on each new line 

Enjoy!

Open for any suggestions or improvements, use issues tab!
