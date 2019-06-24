# LeapM-VX1

Project video: https://www.youtube.com/watch?v=zyG7rsY_ZL8 


Project page: https://gesturelab.blogspot.com/


July 29, 2018
[TUTORIAL] - ENVIRONMENT SETUP GUIDE
(Note: This tutorial uses Visual Studio 2015/2017 version and Voxon SDK v2.8)
<br>
Tutorial Video: https://drive.google.com/file/d/1RvV0048e-wtGQ5SvW4h4JMXTH63YpAJx/view
<br>
Copy the voxiesimp.c, voxiebox.h and voxiebox.dll to your project folder.
<br>
These files can be found in runtime_v2.8 folder.
<br>
voxiebox.dll is compiled in 64-bit mode. Make sure the compilation target is also 64-bit or voxie_load() will fail.
<br>
Copy the LeapC.h file (from LeapSDK → include folder) and the LeapC.dll file (from LeapSDK → lib → x64 folder) to your project folder.
<br>
In order to use x64 native tools command prompt, you have to call it from your Windows start menu.
<br>
Start menu → Type "Developer Command Prompt" → Right-click and open the file location → Enter the VC folder → select x64 native tools command prompt for your Visual Studio version).
<br>
We have to use this command prompt because the default command prompt no longer supports some commands, such as cl, which we need to compile our program. Additionally, since we need to compile our program to 64-bit program, we have to use the 64-bit compiler.
<br>
Go to your project folder through the command prompt.
<br>
In voxiesimp.c file, you can see the makefile commands:<br>
         #Visual C makefile:<br>
         voxiesimp.exe: voxiesimp.c voxiebox.h; cl /TP voxiesimp.c /Ox /MT /link user32.lib<br>
         del voxiesimp.obj<br>
<br><br>
         To compile the code, enter the following command in the x64 command prompt:<br>
         cl /TP voxiesimp.c /Ox /MT /link user32.lib<br>
         or<br>
         nmake voxiesimp.c<br>

<br><br>
<br>
References:<br>
Voxon SDK: http://voxon.co/developer-kit/<br>
LeapMotion SDK: https://developer.leapmotion.com/get-started/<br>
Build C/C++ code on the command line on Windows, https://docs.microsoft.com/en-us/cpp/build/building-on-the-command-line<br>
Usage of CL command, https://msdn.microsoft.com/en-us/library/610ecb4h.aspx<br>
For mingw:<br>
http://www.mingw.org/wiki/specify_the_libraries_for_the_linker_to_use<br>
http://www.mingw.org/wiki/MSVC_and_MinGW_DLLs<br>
Dynamic-Link Libraries, https://docs.microsoft.com/en-us/windows/desktop/Dlls/dynamic-link-libraries<br>
<br>
