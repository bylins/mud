move lib\world.template lib\world
for /r lib %%i in (*.template) do ren %%i %%~ni