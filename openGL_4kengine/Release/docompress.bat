#TODO:
#Range import does not worl?
#/RANGE:C:\Windows\System32\opengl32

#crinkler.exe /OUT:"codepron.exe" "opengl32.lib" "glu32.lib" "winmm.lib" "kernel32.lib" "user32.lib" "gdi32.lib" "winspool.lib" "comdlg32.lib" "advapi32.lib" "shell32.lib" "ole32.lib" "oleaut32.lib" "uuid.lib" "odbc32.lib" "odbccp32.lib" "mzk.obj" "intro.obj" "main_rel.obj" /ENTRY:"entrypoint" /RANGE:opengl32 /COMPMODE:SLOW /HASHSIZE:200 /HASHTRIES:100 /ORDERTRIES:2000 /REPORT:crinklerreport.html

crinkler.exe /OUT:"4k.exe" "opengl32.lib" "winmm.lib" "kernel32.lib" "user32.lib" "gdi32.lib" "winspool.lib" "comdlg32.lib" "advapi32.lib" "shell32.lib" "ole32.lib" "oleaut32.lib" "uuid.lib" "odbc32.lib" "odbccp32.lib" "mzk.obj" "intro.obj" "main_rel.obj" /ENTRY:"entrypoint" /COMPMODE:SLOW /HASHSIZE:200 /HASHTRIES:100 /ORDERTRIES:2000 /REPORT:crinklerreport.html /RANGE:opengl32 /UNSAFEIMPORT
