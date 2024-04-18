@echo on
if "%1"=="" goto error
nasm -f bin %1.asm -l %1.lst -o %1.com
goto end
:error
echo What do you want to Make?
:end

