@ECHO OFF
ECHO %ProgramFiles%
SET INSTDIR="%ProgramFiles%\StickyNotes"
MKDIR %INSTDIR%
XCOPY StickyNotes.exe %INSTDIR%\
XCOPY Notes.db %INSTDIR%\

@ECHO StickyNotes er blevet installeret i dit program bibliotek - tillykke!
PAUSE