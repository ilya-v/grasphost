:a
@cls
@cd %~dp0
@call build
@call .\grasphost.exe com20
@pause
@goto :a