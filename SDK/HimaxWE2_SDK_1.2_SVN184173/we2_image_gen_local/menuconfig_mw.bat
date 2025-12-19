@echo off 
echo F | XCOPY common\cfg\Loader_config.bin tmp_config.bin /Y >nul

set /a feature_flag_package=0
set /a feature_flag_load_config=0
set /a feature_flag_amr=0
set /a feature_flag_gen_ini=0
set /a amr_enable_flag=0
set /a amr_short_dial_enable_flag=0
set /a standalone_enable_flag=0
set /a fr_enable_flag=0
set /a standalone_ver_input=0
set /a audio_enable_flag=0
set /a user_def_enable_flag=0
set /a user_input_cnt=0
set /a first_pll_disable_flag=0
set /a second_pll_disable_flag=0


set /a VERSION_MAJOR = 1		rem MajorNum indicate tool type
set /a VERSION_MINOR = 0		rem MinorNum indicate tool feature update
set /a VERSION_MINI = 0			rem MiniNum indicate tool bug/issue fix

rem ### menu start ###
:menu_list
cls
:load_priv_package_setting
set priv_package_setting_path=ini_folder\priv.ini
set tmp_setting_path=ini_folder\tmp.ini
set key1=tmp_chip_name
set key2=sec_format_name
set key3=ini_sign_forma
set key4=ini_flash_size
set key5=part_number_name
echo off > %tmp_setting_path%

for /f "tokens=1,2 delims=/=" %%i in (%priv_package_setting_path%) do (
	rem echo i = %%i, key1 = %key1%, key2 = %key2%, key3 = %key3%, key4 = %key4%, key5 = %key5%, 
	if "%%j" == "" (
		echo %%i>>%tmp_setting_path%
	) else (
		echo %%i=%%j>>%tmp_setting_path%
	)
    if "%%i"=="%key1%" set %key1%=%%j
	if "%%i"=="%key2%" set %key2%=%%j
	if "%%i"=="%key3%" set %key3%=%%j
	if "%%i"=="%key4%" set %key4%=%%j
	if "%%i"=="%key5%" set %key5%=%%j
)
rem echo tmp_chip_name = %tmp_chip_name%, sec_format_name = %sec_format_name%, ini_sign_forma = %ini_sign_forma%, ini_flash_size = %ini_flash_size%, part_number_name = %part_number_name%
rem pause

::#region Version Display
@echo [HIMAX] ***************************************************************** 
@echo [HIMAX] ****************                              *******************
@echo [HIMAX] ****************     MenuConfig Ver.%VERSION_MAJOR%.%VERSION_MINOR%.%VERSION_MINI%     *******************  
@echo [HIMAX] ****************                              ******************* 
@echo [HIMAX] *****************************************************************
::#endregion


::#region Menu_List
set menu_list_path=ini_folder\menu_list.ini
set /a menu_list_cnt=1
set /a menu_tmp_cnt=1
set /a aa=0
set menu_list_input=NULL

echo [HIMAX] Feature list:
Setlocal EnableDelayedExpansion
for /f "delims=" %%i in (%menu_list_path%) do (
    echo [HIMAX] !menu_list_cnt!. %%i
	set /a menu_list_cnt += 1
)
set /a menu_tmp_cnt = menu_list_cnt - 2

@echo [HIMAX]
set /p menu_list_input="[HIMAX] Please select one feature ( 1 ~ %menu_tmp_cnt% ): "

if not defined menu_list_input (
    @echo [HIMAX] User input can not be NULL please check and try again!
	goto menu_list
)

if %menu_list_input% leq %menu_tmp_cnt% (
	if %menu_list_input% gtr 0 (
		For /f "delims=" %%c in (%menu_list_path%) do (
			Set /a aa += 1
			rem echo [cdc] !aa!. %%c
			if !aa! equ %menu_list_input% set tmp_menu_list=%%c
		)
	) else (
		@echo [HIMAX]
		@echo [HIMAX] Your selection %menu_list_input% is not on the list please check and try again!
		goto menu_list
	)
) else (
	@echo [HIMAX]
	@echo [HIMAX] Your selection %menu_list_input% is not on the list please check and try again!
	goto menu_list
)

@echo [HIMAX] Your selection is "%tmp_menu_list%"
cls

rem if "%tmp_menu_list%" == "Package" (
rem 	goto Package_Start
rem ) else if "%tmp_menu_list%" == "Loader Config (Option)" (
rem 	goto Load_Config_Start
rem ) else if "%tmp_menu_list%" == "Algo Model (Option)" (
rem 	goto Algo_Model_Start
rem ) else if "%tmp_menu_list%" == "User Define (Option)" (
rem 	goto User_Define_Start
rem ) else if "%tmp_menu_list%" == "Generate ini File" (
rem 	if "%amr_enable_flag%" == "1" (
rem 		goto gen_amr_ini
rem 	) else (
rem 		goto gen_normal_ini
rem 	)
rem ) else if "%tmp_menu_list%" == "Dump Current Settings" (
rem 	goto Dump_Sataus_Start
if "%tmp_menu_list%" == "Loader Config (Option)" (
	goto Load_Config_Start
) else if "%tmp_menu_list%" == "Return Without Saving" (
	echo F | XCOPY tmp_config.bin common\cfg\Loader_config.bin /Y >nul
	del /F /S tmp_config.bin >nul
	exit /b 0
) else if "%tmp_menu_list%" == "Save and Exit" (
	del /F /S tmp_config.bin >nul
	exit /b 0
) else (
	@echo [HIMAX] "%tmp_menu_list%" function is not defined!
	goto menu_list
)
@echo [HIMAX]
rem ### menu end ###
::#endregion
 

::#region Package_Start
:Package_Start
rem ### chip_name start ###

::#region chip_name_list
:chip_name_list
Setlocal EnableDelayedExpansion
set chip_name_path=ini_folder\chip_name.ini
@echo [HIMAX] -------------------- Chip Name List --------------------- 
set /a chip_name_cnt=1
set /a chip_tmp_cnt=1
rem set chip_name_input=NULL
Set /a aa=0
for /f "delims=" %%i in (%chip_name_path%) do (
    echo [HIMAX] !chip_name_cnt!. %%i
	set /a chip_name_cnt += 1
)
set /a chip_tmp_cnt = chip_name_cnt - 2

@echo [HIMAX]
set /p chip_name_input="[HIMAX] Please select one option ( 1 ~ %chip_tmp_cnt% ): "

if not defined chip_name_input (
    @echo [HIMAX] Your selection can not be NULL please check and try again!
	goto chip_name_list
)

if %chip_name_input% leq %chip_tmp_cnt% (
	if %chip_name_input% gtr 0 (
		For /f "delims=" %%c in (%chip_name_path%) do (
			Set /a aa += 1
			rem echo [cdc] !aa!. %%c
			if !aa! equ %chip_name_input% set tmp_chip_name=%%c
		)
	) else (
		@echo [HIMAX]
		@echo [HIMAX] Your selection %chip_name_input% is not on the list please check and try again!
		goto chip_name_list
	)
) else (
	@echo [HIMAX]
	@echo [HIMAX] Your selection %chip_name_input% is not on the list please check and try again!
	goto chip_name_list
)

@echo [HIMAX] Your selection is "%tmp_chip_name%"
echo off > %priv_package_setting_path%
for /f "tokens=1,2 delims=/=" %%i in (%tmp_setting_path%) do (
	if "%%j" == "" (
		echo %%i>>%priv_package_setting_path%
	) else if "%%i"=="%key1%" (
		echo %%i=%tmp_chip_name%>>%priv_package_setting_path%
	) else (
		echo %%i=%%j>>%priv_package_setting_path%
	)
)
echo off > %tmp_setting_path%
for /f "tokens=1,2 delims=/=" %%i in (%priv_package_setting_path%) do (
	if "%%j" == "" (
		echo %%i>>%tmp_setting_path%
	) else (
		echo %%i=%%j>>%tmp_setting_path%
	)
)
rem cls
rem ### chip_name end ###

@echo [HIMAX]
::#endregion

::#region sec_format

rem ### sec_format start ###
Setlocal EnableDelayedExpansion
set sec_format_path=ini_folder\sec_format.ini
@echo [HIMAX] -------------------- Sec Format List -------------------- 

:sec_format_list
set /a sec_format=1
set /a sec_format_cnt=1
rem set sec_format_input=NULL
Set /a bb=0
for /f "delims=" %%i in (%sec_format_path%) do (
    echo [HIMAX] !sec_format!. %%i
	set /a sec_format += 1
)
set /a sec_format_cnt = sec_format - 2

@echo [HIMAX]
set /p sec_format_input="[HIMAX] Please select one option ( 1 ~ %sec_format_cnt% ): "

if not defined sec_format_input (
    @echo [HIMAX] Your selection can not be NULL please check and try again!
	goto sec_format_list
)

if %sec_format_input% leq %sec_format_cnt% (
	if %sec_format_input% gtr 0 (
		For /f "delims=" %%c in (%sec_format_path%) do (
			Set /a bb += 1
			if !bb! equ %sec_format_input% set sec_format_name=%%c
		)
	) else (
		@echo [HIMAX]
		@echo [HIMAX] Your selection %sec_format_input% is not on the list please check and try again!
		goto sec_format_list
	)
) else (
	@echo [HIMAX]
	@echo [HIMAX] Your selection %sec_format_input% is not on the list please check and try again!
	goto sec_format_list
)

@echo [HIMAX] Your selection is "%sec_format_name%"
if %sec_format_input% equ 1 (
	set sec_format_name=BLp
	set ini_sign_forma=sign_formal
) else if %sec_format_input% equ 2 (
	set sec_format_name=BLw
	set ini_sign_forma=enc_formal2
) else (
	set sec_format_name=BLp
	set ini_sign_forma=sign_formal
)
echo off > %priv_package_setting_path%
for /f "tokens=1,2 delims=/=" %%i in (%tmp_setting_path%) do (
	if "%%j" == "" (
		echo %%i>>%priv_package_setting_path%
	) else if "%%i"=="%key2%" (
		echo %%i=%sec_format_name%>>%priv_package_setting_path%
	) else if "%%i"=="%key3%" (
		echo %%i=%ini_sign_forma%>>%priv_package_setting_path%
	) else (
		echo %%i=%%j>>%priv_package_setting_path%
	)
)
echo off > %tmp_setting_path%
for /f "tokens=1,2 delims=/=" %%i in (%priv_package_setting_path%) do (
	if "%%j" == "" (
		echo %%i>>%tmp_setting_path%
	) else (
		echo %%i=%%j>>%tmp_setting_path%
	)
)
rem ### sec_format end ###

@echo [HIMAX]
::#endregion

::#region flash_max_size

rem ### flash_max_size start ###
Setlocal EnableDelayedExpansion
set flash_max_size_path=ini_folder\flash_max_size.ini
@echo [HIMAX] ------------------ Max Flash Size List ------------------ 

:flash_max_size_list
set /a flash_max_size=1
set /a flash_max_size_cnt=1
set flash_max_size_input=NULL
Set /a cc=0
for /f "delims=" %%i in (%flash_max_size_path%) do (
    echo [HIMAX] !flash_max_size!. %%i
	set /a flash_max_size += 1
)
set /a flash_max_size_cnt = flash_max_size - 2

@echo [HIMAX]
set /p flash_max_size_input="[HIMAX] Please select one option ( 1 ~ %flash_max_size_cnt% ): "

if not defined flash_max_size_input (
    @echo [HIMAX] Your selection can not be NULL please check and try again!
	goto flash_max_size_list
)

if %flash_max_size_input% leq %flash_max_size_cnt% (
	if %flash_max_size_input% gtr 0 (
		For /f "delims=" %%c in (%flash_max_size_path%) do (
			Set /a cc += 1
			if !cc! equ %flash_max_size_input% set flash_max_size_name=%%c
		)
	) else (
		@echo [HIMAX]
		@echo [HIMAX] Your selection %flash_max_size_input% is not on the list please check and try again!
		goto flash_max_size_list
	)
) else (
	@echo [HIMAX]
	@echo [HIMAX] Your selection %flash_max_size_input% is not on the list please check and try again!
	goto flash_max_size_list
)

@echo [HIMAX] Your selection is "%flash_max_size_name%"
if %flash_max_size_input% equ 1 (set ini_flash_size=0x100000) else if %flash_max_size_input% equ 2 (set ini_flash_size=0x200000) else (set ini_flash_size=0x100000)
echo off > %priv_package_setting_path%
for /f "tokens=1,2 delims=/=" %%i in (%tmp_setting_path%) do (
	if "%%j" == "" (
		echo %%i>>%priv_package_setting_path%
	) else if "%%i"=="%key4%" (
		echo %%i=%ini_flash_size%>>%priv_package_setting_path%
	) else (
		echo %%i=%%j>>%priv_package_setting_path%
	)
)
echo off > %tmp_setting_path%
for /f "tokens=1,2 delims=/=" %%i in (%priv_package_setting_path%) do (
	if "%%j" == "" (
		echo %%i>>%tmp_setting_path%
	) else (
		echo %%i=%%j>>%tmp_setting_path%
	)
)
rem ### flash_max_size end ###

@echo [HIMAX]
::#endregion

::#region part_number

rem ### part_number start ###
if %chip_name_input% equ 1 set chip_pin_name=part_number_WLCSP38.ini
if %chip_name_input% equ 2 set chip_pin_name=part_number_QFN46.ini
if %chip_name_input% equ 3 set chip_pin_name=part_number_QFN72.ini
if %chip_name_input% equ 4 set chip_pin_name=part_number_LQFP128.ini

Setlocal EnableDelayedExpansion
set part_number_path=ini_folder\part_number\%chip_pin_name%
@echo [HIMAX] ---------------- %tmp_chip_name% Part Number List ----------------- 

:part_number_list
set /a part_number=1
set /a part_number_cnt=1
set part_number_input=NULL
Set /a ee=0
for /f "delims=" %%i in (%part_number_path%) do (
    echo [HIMAX] !part_number!. %%i
	set /a part_number += 1
)
set /a part_number_cnt = part_number - 2

@echo [HIMAX]
set /p part_number_input="[HIMAX] Please select one option ( 1 ~ %part_number_cnt% ): "

if not defined part_number_input (
    @echo [HIMAX] Your selection can not be NULL please check and try again!
	goto part_number_list
)

if %part_number_input% leq %part_number_cnt% (
	if %part_number_input% gtr 0 (
		For /f "delims=" %%c in (%part_number_path%) do (
			Set /a ee += 1
			if !ee! equ %part_number_input% set part_number_name=%%c
		)
	) else (
		@echo [HIMAX]
		@echo [HIMAX] Your selection %part_number_input% is not on the list please check and try again!
		goto part_number_list
	)
) else (
	@echo [HIMAX]
	@echo [HIMAX] Your selection %part_number_input% is not on the list please check and try again!
	goto part_number_list
)

@echo [HIMAX] Your selection is "%part_number_name%"
echo off > %priv_package_setting_path%
for /f "tokens=1,2 delims=/=" %%i in (%tmp_setting_path%) do (
	if "%%j" == "" (
		echo %%i>>%priv_package_setting_path%
	) else if "%%i"=="%key5%" (
		echo %%i=%part_number_name%>>%priv_package_setting_path%
	) else (
		echo %%i=%%j>>%priv_package_setting_path%
	)
)
echo off > %tmp_setting_path%
for /f "tokens=1,2 delims=/=" %%i in (%priv_package_setting_path%) do (
	if "%%j" == "" (
		echo %%i>>%tmp_setting_path%
	) else (
		echo %%i=%%j>>%tmp_setting_path%
	)
)
rem ### part_number end ###

@echo [HIMAX]
goto menu_list
::#endregion

::#endregion


::#region Loader_Config_Start
rem ### load config start ###
:Load_Config_Start
set /a boot_loader_value=1
set /a boot_loader_cnt=1
Set /a gg=0
set first_2nd_loader_input=NULL
set boot_loader_item_path=ini_folder\boot_loader\loader_options.ini

set /a first_bootloader_cnt=1
rem set first_loader_input=NULL
set first_bootloader_path=ini_folder\boot_loader\first_bootloader.ini

set /a clock_case_cnt=1
set clock_case_path=ini_folder\boot_loader\clock_case.ini

set /a clock_type_cnt=1
set clock_type_path=ini_folder\boot_loader\clock_type.ini

set /a other_flag_cnt=1
set other_flag_path=ini_folder\boot_loader\other_flag.ini

set /a first_cpu_frequency_cnt=1
set cpu_frequency_path=ini_folder\boot_loader\cpu_frequency.ini

set /a second_bootloader_cnt=1
set second_bootloader_path=ini_folder\boot_loader\second_bootloader.ini

set /a cmd_uart1_pin_mux_cnt=1
set second_cmd_uart1_pin_mux_path=ini_folder\boot_loader\cmd_uart1_pin_mux.ini

set /a second_re_encryption_cnt=1

set /a second_clock_source_cnt=1

set /a second_cpu_frequency_cnt=1
Setlocal EnableDelayedExpansion

cls
@echo [HIMAX] ------------------- Boot Loader List -------------------- 
for /f "delims=" %%i in (%boot_loader_item_path%) do (
    echo [HIMAX] !boot_loader_value!. %%i
	set /a boot_loader_value += 1
)
set /a boot_loader_cnt = boot_loader_value - 2

@echo [HIMAX]
set /p first_2nd_loader_input="[HIMAX] Please select loader config ( 1 ~ %boot_loader_cnt% ): "

if not defined first_2nd_loader_input (
    @echo [HIMAX] Your selection can not be NULL please check and try again!
	goto Load_Config_Start
)

if %first_2nd_loader_input% leq %boot_loader_cnt% (
	if %first_2nd_loader_input% gtr 0 (
		For /f "delims=" %%c in (%boot_loader_item_path%) do (
			Set /a gg += 1
			if !gg! equ %first_2nd_loader_input% set loader_name=%%c
		)
	) else (
		@echo [HIMAX]
		@echo [HIMAX] Your selection %first_2nd_loader_input% is not on the list please check and try again!
		goto Load_Config_Start
	)
) else (
	@echo [HIMAX]
	@echo [HIMAX] Your selection %first_2nd_loader_input% is not on the list please check and try again!
	goto Load_Config_Start
)

@echo [HIMAX] Your selection is "%loader_name%"
@echo [HIMAX]
if %first_2nd_loader_input% equ 1 (
	goto 1st_bootloader_selection
) else if %first_2nd_loader_input% equ 2 (
	goto 2nd_bootloader_selection
) else if %first_2nd_loader_input% equ 3 (
	goto menu_list
)

::#region 1st Boot Loader List
:1st_bootloader_selection
set /a first_bootloader_value=1
set first_loader_input=NULL
Set /a ga=0

echo [HIMAX] ------------------ First Boot Loader -------------------- 
for /f "delims=" %%i in (%first_bootloader_path%) do (
	echo [HIMAX] !first_bootloader_value!. %%i
	set /a first_bootloader_value += 1
)
set /a first_bootloader_cnt = !first_bootloader_value! - 2

@echo [HIMAX] 
set /p first_loader_input="[HIMAX] Please select one option ( 1 ~ !first_bootloader_cnt! ): "

if not defined first_loader_input (
	@echo [HIMAX] Your selection can not be NULL please check and try again!
	goto 1st_bootloader_selection
)

if %first_loader_input% leq %first_bootloader_cnt% (
	if %first_loader_input% gtr 0 (
		For /f "delims=" %%c in (%first_bootloader_path%) do (
			Set /a ga += 1
			if !ga! equ %first_loader_input% set first_loader_name=%%c
		)
	) else (
		@echo [HIMAX]
		@echo [HIMAX] Your selection %first_loader_input% is not on the list please check and try again!
		goto 1st_bootloader_selection
	)
) else (
	@echo [HIMAX]
	@echo [HIMAX] Your selection %first_loader_input% is not on the list please check and try again!
	goto 1st_bootloader_selection
)
echo [HIMAX] Your selection is "%first_loader_name%"

if "%first_loader_input%" == "1" (
	goto first_clock_case_list
) else if "%first_loader_input%" == "2" (
	goto first_clock_type_list
) else if "%first_loader_input%" == "3" (
	goto first_other_flag_list
) else if "%first_loader_input%" == "4" (
	goto first_cpu_frequency_list
) else if "%first_loader_input%" == "5" (
	goto Load_Config_Start
)
::#endregion


::#region 1st Boot Loader Clock Case
rem ### 1st boot loader Clock Case start ###
:first_clock_case_list
set /a first_clock_case_value=1
set first_clock_case_input=NULL
Set /a ad=0

@echo [HIMAX]
@echo [HIMAX] ----------------- 1st Boot Loader Clock Case List ------------------- 
for /f "delims=" %%i in (%clock_case_path%) do (
	echo [HIMAX] !first_clock_case_value!. %%i
	set /a first_clock_case_value += 1
)
set /a clock_case_cnt = first_clock_case_value - 2

@echo [HIMAX]
set /p first_clock_case_input="[HIMAX] Please select options ( 1 ~ %clock_case_cnt% ): "

if not defined first_clock_case_input (
	@echo [HIMAX] Your selection can not be NULL please check and try again!
	goto first_clock_case_list
)

if %first_clock_case_input% leq %clock_case_cnt% (
	if %first_clock_case_input% gtr 0 (
		For /f "delims=" %%m in (%clock_case_path%) do (
			Set /a ad += 1
			if !ad! equ %first_clock_case_input% set first_clock_case_name=%%m
		)
	) else (
		@echo [HIMAX]
		@echo [HIMAX] Your selection %first_clock_case_input% is not on the list please check and try again!
		goto first_clock_case_list
	)
) else (
	@echo [HIMAX]
	@echo [HIMAX] Your selection %first_clock_case_input% is not on the list please check and try again!
	goto first_clock_case_list
)

@echo [HIMAX] Set 1st boot loader clock case "%first_clock_case_name%" finished!
@echo [HIMAX]

if "%first_clock_case_input%" == "1" (
	Set "content_value_mask=00"
	goto set_first_clock_case_bin
) else if "%first_clock_case_input%" == "2" (
	Set "content_value_mask=01"
	goto set_first_clock_case_bin
) else if "%first_clock_case_input%" == "3" (
	Set "content_value_mask=02"
	goto set_first_clock_case_bin
) else if "%first_clock_case_input%" == "4" (
	Set "content_value_mask=03"
	goto set_first_clock_case_bin
) else if "%first_clock_case_input%" == "5" (
	Set "content_value_mask=04"
	goto set_first_clock_case_bin
) else if "%first_clock_case_input%" == "6" (
	Set "content_value_mask=05"
	goto set_first_clock_case_bin
) else if "%first_clock_case_input%" == "7" (
	Set "content_value_mask=06"
	goto set_first_clock_case_bin
) else if "%first_clock_case_input%" == "8" (
	Set "content_value_mask=07"
	goto set_first_clock_case_bin
) else if "%first_clock_case_input%" == "9" (
	Set "content_value_mask=08"
	goto set_first_clock_case_bin
) else if "%first_clock_case_input%" == "10" (
	Set "content_value_mask=09"
	goto set_first_clock_case_bin
) else if "%first_clock_case_input%" == "11" (
	Set "content_value_mask=0A"
	goto set_first_clock_case_bin
) else if "%first_clock_case_input%" == "12" (
	Set "content_value_mask=0B"
	goto set_first_clock_case_bin
) else if "%first_clock_case_input%" == "13" (
	goto 1st_bootloader_selection
)
:set_first_clock_case_bin
certutil -f -encodehex common\cfg\Loader_config.bin Loader_config.txt >nul
del common\cfg\Loader_config.bin 
Set /a ae=0
set "content_value="
For /f "tokens=2-17" %%a in (Loader_config.txt) do (
	Set /a ae += 1
	if !ae! equ 1 (
		rem echo sam_shih %%a, %%b, %%c, %%d, !content_value_mask!, %%f, %%g, %%h, %%i, %%j, %%k, %%l, %%m, %%n, %%o, %%p
		set "content_value=%%a %%b %%c %%d !content_value_mask! %%f %%g %%h %%i %%j %%k %%l %%m %%n %%o %%p"
	) else (
		set "content_value=%%a %%b %%c %%d %%e %%f %%g %%h %%i %%j %%k %%l %%m %%n %%o %%p"
	)
	>>temp.txt echo(!content_value!
)
certutil -f -decodehex temp.txt common\cfg\Loader_config.bin >nul
del temp.txt
goto 1st_bootloader_selection
rem ### 1st boot loader Clock Case end ###
::#endregion


::#region 1st Boot Loader Clock Type
rem ### 1st boot loader Clock Type start ###
:first_clock_type_list
set /a first_clock_type_value=1
set first_clock_type_input=NULL
Set /a ad=0

@echo [HIMAX]
@echo [HIMAX] ----------------- 1st Boot Loader Clock Type List ------------------- 
for /f "delims=" %%i in (%clock_type_path%) do (
	echo [HIMAX] !first_clock_type_value!. %%i
	set /a first_clock_type_value += 1
)
set /a clock_type_cnt = first_clock_type_value - 2

@echo [HIMAX]
set /p first_clock_type_input="[HIMAX] Please select options ( 1 ~ %clock_type_cnt% ): "

if not defined first_clock_type_input (
	@echo [HIMAX] Your selection can not be NULL please check and try again!
	goto first_clock_type_list
)

if %first_clock_type_input% leq %clock_type_cnt% (
	if %first_clock_type_input% gtr 0 (
		For /f "delims=" %%m in (%clock_type_path%) do (
			Set /a ad += 1
			if !ad! equ %first_clock_type_input% set first_clock_type_name=%%m
		)
	) else (
		@echo [HIMAX]
		@echo [HIMAX] Your selection %first_clock_type_input% is not on the list please check and try again!
		goto first_clock_type_list
	)
) else (
	@echo [HIMAX]
	@echo [HIMAX] Your selection %first_clock_type_input% is not on the list please check and try again!
	goto first_clock_type_list
)

@echo [HIMAX] Set 1st boot loader clock type "%first_clock_type_name%" finished!
@echo [HIMAX]

if "%first_clock_type_input%" == "1" (
	set /a first_pll_disable_flag=1
	Set "content_value_mask=00"
	goto set_first_clock_type_bin
) else if "%first_clock_type_input%" == "2" (
	set /a first_pll_disable_flag=0
	Set "content_value_mask=01"
	goto set_first_clock_type_bin
) else if "%first_clock_type_input%" == "3" (
	goto 1st_bootloader_selection
)
:set_first_clock_type_bin
certutil -f -encodehex common\cfg\Loader_config.bin Loader_config.txt >nul
del common\cfg\Loader_config.bin 
Set /a ae=0
set "content_value="
For /f "tokens=2-17" %%a in (Loader_config.txt) do (
	Set /a ae += 1
	if !ae! equ 1 (
		rem echo sam_shih %%a, %%b, %%c, %%d, %%e, !content_value_mask!, %%g, %%h, %%i, %%j, %%k, %%l, %%m, %%n, %%o, %%p
		set "content_value=%%a %%b %%c %%d %%e !content_value_mask! %%g %%h %%i %%j %%k %%l %%m %%n %%o %%p"
	) else (
		set "content_value=%%a %%b %%c %%d %%e %%f %%g %%h %%i %%j %%k %%l %%m %%n %%o %%p"
	)
	>>temp.txt echo(!content_value!
)
certutil -f -decodehex temp.txt common\cfg\Loader_config.bin >nul
del temp.txt
goto 1st_bootloader_selection
rem ### 1st boot loader Clock Case end ###
::#endregion


::#region 1st Boot Loader Other Flag
rem ### 1st boot loader Other Flag start ###
:first_other_flag_list
set /a first_other_flag_value=1
set first_other_flag_input=NULL
Set /a ad=0

@echo [HIMAX]
@echo [HIMAX] ----------------- 1st Boot Loader Other Flag List ------------------- 
for /f "delims=" %%i in (%other_flag_path%) do (
	echo [HIMAX] !first_other_flag_value!. %%i
	set /a first_other_flag_value += 1
)
set /a other_flag_cnt = first_other_flag_value - 2

@echo [HIMAX]
set /p first_other_flag_input="[HIMAX] Please select options ( 1 ~ %other_flag_cnt% ): "

if not defined first_other_flag_input (
	@echo [HIMAX] Your selection can not be NULL please check and try again!
	goto first_other_flag_list
)

if %first_other_flag_input% leq %other_flag_cnt% (
	if %first_other_flag_input% gtr 0 (
		For /f "delims=" %%m in (%other_flag_path%) do (
			Set /a ad += 1
			if !ad! equ %first_other_flag_input% set first_other_flag_name=%%m
		)
	) else (
		@echo [HIMAX]
		@echo [HIMAX] Your selection %first_other_flag_input% is not on the list please check and try again!
		goto first_other_flag_list
	)
) else (
	@echo [HIMAX]
	@echo [HIMAX] Your selection %first_other_flag_input% is not on the list please check and try again!
	goto first_other_flag_list
)

@echo [HIMAX] Set 1st boot loader other flag "%first_other_flag_name%" finished!
@echo [HIMAX]

if "%first_other_flag_input%" == "1" (
	Set "content_value_mask=00"
	goto set_first_other_flag_bin
) else if "%first_other_flag_input%" == "2" (
	Set "content_value_mask=01"
	goto set_first_other_flag_bin
) else if "%first_other_flag_input%" == "3" (
	Set "content_value_mask=02"
	goto set_first_other_flag_bin
) else if "%first_other_flag_input%" == "4" (
	goto 1st_bootloader_selection
)
:set_first_other_flag_bin
certutil -f -encodehex common\cfg\Loader_config.bin Loader_config.txt >nul
del common\cfg\Loader_config.bin 
Set /a ae=0
set "content_value="
For /f "tokens=2-17" %%a in (Loader_config.txt) do (
	Set /a ae += 1
	if !ae! equ 1 (
		rem echo sam_shih %%a, %%b, %%c, %%d, %%e, %%f, !content_value_mask!, %%h, %%i, %%j, %%k, %%l, %%m, %%n, %%o, %%p
		set "content_value=%%a %%b %%c %%d %%e %%f !content_value_mask! %%h %%i %%j %%k %%l %%m %%n %%o %%p"
	) else (
		set "content_value=%%a %%b %%c %%d %%e %%f %%g %%h %%i %%j %%k %%l %%m %%n %%o %%p"
	)
	>>temp.txt echo(!content_value!
)
certutil -f -decodehex temp.txt common\cfg\Loader_config.bin >nul
del temp.txt
goto 1st_bootloader_selection
rem ### 1st boot loader Other Flag end ###
::#endregion


::#region 1st Boot Loader CPU Frequency
rem ### 1st boot loader CPU Frequency start ###
:first_cpu_frequency_list
set /a first_cpu_frequency_value=1
set first_cpu_frequency_input=NULL
Set /a ad=0

@echo [HIMAX]
@echo [HIMAX] ----------------- 1st Boot Loader CPU Frequency List ------------------- 
for /f "delims=" %%i in (%cpu_frequency_path%) do (
	echo [HIMAX] !first_cpu_frequency_value!. %%i
	set /a first_cpu_frequency_value += 1
)
set /a first_cpu_frequency_cnt = first_cpu_frequency_value - 2

@echo [HIMAX]
set /p first_cpu_frequency_input="[HIMAX] Please select options ( 1 ~ %first_cpu_frequency_cnt% ): "

if not defined first_cpu_frequency_input (
	@echo [HIMAX] Your selection can not be NULL please check and try again!
	goto first_cpu_frequency_list
)

if %first_cpu_frequency_input% leq %first_cpu_frequency_cnt% (
	if %first_cpu_frequency_input% gtr 0 (
		For /f "delims=" %%m in (%cpu_frequency_path%) do (
			Set /a ad += 1
			if !ad! equ %first_cpu_frequency_input% set first_cpu_frequency_name=%%m
		)
	) else (
		@echo [HIMAX]
		@echo [HIMAX] Your selection %first_cpu_frequency_input% is not on the list please check and try again!
		goto first_cpu_frequency_list
	)
) else (
	@echo [HIMAX]
	@echo [HIMAX] Your selection %first_cpu_frequency_input% is not on the list please check and try again!
	goto first_cpu_frequency_list
)

@echo [HIMAX] Set 1st boot loader CPU frequency "%first_cpu_frequency_name%" finished!
@echo [HIMAX]

rem 100M
if "%first_cpu_frequency_input%" == "1" (
	Set "content_value_mask=00"
	Set "content_value_mask_f=E1"
	Set "content_value_mask_n=F5"
	Set "content_value_mask_t=05"
	goto set_first_cpu_frequency_bin
rem 150M
) else if "%first_cpu_frequency_input%" == "2" (
	Set "content_value_mask=80"
	Set "content_value_mask_f=D1"
	Set "content_value_mask_n=F0"
	Set "content_value_mask_t=08"
	goto set_first_cpu_frequency_bin
rem 200M
) else if "%first_cpu_frequency_input%" == "3" (
	Set "content_value_mask=00"
	Set "content_value_mask_f=C2"
	Set "content_value_mask_n=EB"
	Set "content_value_mask_t=0B"
	goto set_first_cpu_frequency_bin
rem 300M
) else if "%first_cpu_frequency_input%" == "4" (
	Set "content_value_mask=00"
	Set "content_value_mask_f=A3"
	Set "content_value_mask_n=E1"
	Set "content_value_mask_t=11"
	goto set_first_cpu_frequency_bin
rem 350M
) else if "%first_cpu_frequency_input%" == "5" (
	Set "content_value_mask=80"
	Set "content_value_mask_f=93"
	Set "content_value_mask_n=DC"
	Set "content_value_mask_t=14"
	goto set_first_cpu_frequency_bin
rem 400M
) else if "%first_cpu_frequency_input%" == "6" (
	Set "content_value_mask=00"
	Set "content_value_mask_f=84"
	Set "content_value_mask_n=D7"
	Set "content_value_mask_t=17"
	goto set_first_cpu_frequency_bin
rem 450M
) else if "%first_cpu_frequency_input%" == "7" (
	Set "content_value_mask=80"
	Set "content_value_mask_f=74"
	Set "content_value_mask_n=D2"
	Set "content_value_mask_t=1A"
	goto set_first_cpu_frequency_bin
) else if "%first_cpu_frequency_input%" == "8" (
	goto 1st_bootloader_selection
)
:set_first_cpu_frequency_bin
certutil -f -encodehex common\cfg\Loader_config.bin Loader_config.txt >nul
del common\cfg\Loader_config.bin 
Set /a ae=0
set "content_value="
For /f "tokens=2-17" %%a in (Loader_config.txt) do (
	Set /a ae += 1
	if !ae! equ 1 (
		rem echo sam_shih %%a, %%b, %%c, %%d, %%e, %%f, %%g, %%h, !content_value_mask!, !content_value_mask_f!, !content_value_mask_n!, !content_value_mask_t!, %%m, %%n, %%o, %%p
		set "content_value=%%a %%b %%c %%d %%e %%f %%g %%h !content_value_mask! !content_value_mask_f! !content_value_mask_n! !content_value_mask_t! %%m %%n %%o %%p"
	) else (
		set "content_value=%%a %%b %%c %%d %%e %%f %%g %%h %%i %%j %%k %%l %%m %%n %%o %%p"
	)
	>>temp.txt echo(!content_value!
)
certutil -f -decodehex temp.txt common\cfg\Loader_config.bin >nul
del temp.txt
goto first_pll_setting_list
rem ### 1st boot loader CPU Frequency end ###
::#endregion


::#region 1st Boot PLL Setting
rem ### 1st boot loader pll_setting start ###
:first_pll_setting_list
for /L %%i in (1,1,%length%) do set /a ret*=%ten_dec%
for /L %%g in (1,1,24) do set pll_%%g_value=NULL

rem common values
Set "pll_3_value=61"
Set "pll_4_value=38"
Set "pll_5_value=8F"
Set "pll_6_value=83"
Set "pll_7_value=D1"
Set "pll_8_value=C0"
Set "pll_9_value=16"
Set "pll_14_value=D0"
Set "pll_16_value=C0"
Set "pll_19_value=00"
Set "pll_20_value=0A"
Set "pll_21_value=C0"
Set "pll_22_value=FE"
Set "pll_23_value=00"
Set "pll_24_value=00"

if %first_cpu_frequency_input% equ 1 (
	rem echo 1st cpu freq is 100MHz
	if %first_pll_disable_flag% equ 0 (
		rem default enable clock type
		Set "pll_1_value=43"
		Set "pll_2_value=28"
		Set "pll_10_value=32"
		Set "pll_11_value=90"
		Set "pll_12_value=04"
		Set "pll_13_value=00"
		Set "pll_15_value=73"
		Set "pll_17_value=08"
		Set "pll_18_value=00"
	) else (
		rem disable clock type
		Set "pll_1_value=03"
		Set "pll_2_value=28"
		Set "pll_10_value=32"
		Set "pll_11_value=80"
		Set "pll_12_value=10"
		Set "pll_13_value=00"
		Set "pll_15_value=CB"
		Set "pll_17_value=21"
		Set "pll_18_value=00"
	)
) else if %first_cpu_frequency_input% equ 2 (
	rem echo 1st cpu freq is 150MHz
	Set "pll_1_value=43"
	Set "pll_2_value=28"
	Set "pll_10_value=4B"
	Set "pll_11_value=90"
	Set "pll_12_value=06"
	Set "pll_13_value=00"
	Set "pll_15_value=AC"
	Set "pll_17_value=0C"
	Set "pll_18_value=00"
) else if %first_cpu_frequency_input% equ 3 (
	rem echo 1st cpu freq is 200MHz
	Set "pll_1_value=43"
	Set "pll_2_value=18"
	Set "pll_10_value=32"
	Set "pll_11_value=90"
	Set "pll_12_value=04"
	Set "pll_13_value=00"
	Set "pll_15_value=73"
	Set "pll_17_value=08"
	Set "pll_18_value=00"
) else if %first_cpu_frequency_input% equ 4 (
	rem echo 1st cpu freq is 300MHz
	Set "pll_1_value=43"
	Set "pll_2_value=18"
	Set "pll_10_value=4B"
	Set "pll_11_value=90"
	Set "pll_12_value=06"
	Set "pll_13_value=00"
	Set "pll_15_value=AC"
	Set "pll_17_value=0C"
	Set "pll_18_value=00"
) else if %first_cpu_frequency_input% equ 5 (
	rem echo 1st cpu freq is 350MHz
	Set "pll_1_value=43"
	Set "pll_2_value=18"
	Set "pll_10_value=58"
	Set "pll_11_value=90"
	Set "pll_12_value=07"
	Set "pll_13_value=0A"
	Set "pll_15_value=DE"
	Set "pll_17_value=0E"
	Set "pll_18_value=04"
) else if %first_cpu_frequency_input% equ 6 (
	rem echo 1st cpu freq is 400MHz
	Set "pll_1_value=43"
	Set "pll_2_value=10"
	Set "pll_10_value=32"
	Set "pll_11_value=90"
	Set "pll_12_value=04"
	Set "pll_13_value=00"
	Set "pll_15_value=73"
	Set "pll_17_value=08"
	Set "pll_18_value=00"
) else if %first_cpu_frequency_input% equ 7 (
	rem echo 1st cpu freq is 450MHz
	Set "pll_1_value=43"
	Set "pll_2_value=10"
	Set "pll_10_value=39"
	Set "pll_11_value=80"
	Set "pll_12_value=04"
	Set "pll_13_value=8F"
	Set "pll_15_value=A1"
	Set "pll_17_value=08"
	Set "pll_18_value=02"
)

certutil -f -encodehex common\cfg\Loader_config.bin Loader_config.txt >nul
del common\cfg\Loader_config.bin 
set "content_value="
Set /a cf=0
For /f "tokens=2-17" %%a in (Loader_config.txt) do (
	Set /a cf += 1
	if !cf! equ 1 (
		rem echo sam_shih %%a, %%b, %%c, %%d, %%e, %%f, %%g, %%h, %%i, %%j, %%k, %%l, !pll_1_value!, !pll_2_value!, !pll_3_value!, !pll_4_value!
		set "%%a %%b %%c %%d %%e %%f %%g %%h %%i %%j %%k %%l !pll_1_value! !pll_2_value! !pll_3_value! !pll_4_value!"
	) else if !cf! equ 2 (		
		rem echo sam_shih !pll_5_value!, !pll_6_value!, !pll_7_value!, !pll_8_value!, !pll_9_value!, !pll_10_value!, !pll_11_value!, !pll_12_value!, !pll_13_value!, !pll_14_value!, !pll_15_value!, !pll_16_value!, !pll_17_value!, !pll_18_value!, !pll_19_value!, !pll_20_value!
		set "content_value=!pll_5_value! !pll_6_value! !pll_7_value! !pll_8_value! !pll_9_value! !pll_10_value! !pll_11_value! !pll_12_value! !pll_13_value! !pll_14_value! !pll_15_value! !pll_16_value! !pll_17_value! !pll_18_value! !pll_19_value! !pll_20_value!"
	)  else if !cf! equ 3 (		
		rem echo sam_shih !pll_21_value!, !pll_22_value!, !pll_23_value!, !pll_24_value!, %%e, %%f, %%g, %%h, %%i, %%j, %%k, %%l, %%m, %%n, %%o, %%p
		set "content_value=!pll_21_value! !pll_22_value! !pll_23_value! !pll_24_value! %%e %%f %%g %%h %%i %%j %%k %%l %%m %%n %%o %%p"
	) else (
		rem echo sam_shihhh %%a, %%b, %%c, %%d, %%e, %%f, %%g, %%h, %%i, %%j, %%k, %%l, %%m, %%n, %%o, %%p
		set "content_value=%%a %%b %%c %%d %%e %%f %%g %%h %%i %%j %%k %%l %%m %%n %%o %%p"
	)
	>>temp.txt echo(!content_value!
)
certutil -f -decodehex temp.txt common\cfg\Loader_config.bin >nul
del temp.txt
goto 1st_bootloader_selection
rem ### 1st boot loader pll_setting end ###
::#endregion


::#region 2nd Boot Loader List
:2nd_bootloader_selection
set /a second_bootloader_value=1
set second_loader_input=NULL
Set /a gb=0

echo [HIMAX] ------------------ Second Boot Loader ------------------- 
for /f "delims=" %%i in (%second_bootloader_path%) do (
	echo [HIMAX] !second_bootloader_value!. %%i
	set /a second_bootloader_value += 1
)
set /a second_bootloader_cnt = !second_bootloader_value! - 2

@echo [HIMAX] 
set /p second_loader_input="[HIMAX] Please select one option ( 1 ~ !second_bootloader_cnt! ): "

if not defined second_loader_input (
	@echo [HIMAX] Your selection can not be NULL please check and try again!
	goto 2nd_bootloader_selection
)

if %second_loader_input% leq %second_bootloader_cnt% (
	if %second_loader_input% gtr 0 (
		For /f "delims=" %%c in (%second_bootloader_path%) do (
			Set /a gb += 1
			if !gb! equ %second_loader_input% set second_loader_name=%%c
		)
	) else (
		@echo [HIMAX]
		@echo [HIMAX] Your selection %second_loader_input% is not on the list please check and try again!
		goto 2nd_bootloader_selection
	)
) else (
	@echo [HIMAX]
	@echo [HIMAX] Your selection %second_loader_input% is not on the list please check and try again!
	goto 2nd_bootloader_selection
)
echo [HIMAX] Your selection is "%second_loader_name%"

if "%second_loader_input%" == "1" (
	goto second_clock_case_list
) else if "%second_loader_input%" == "2" (
	goto second_clock_type_list
) else if "%second_loader_input%" == "3" (
	goto second_other_flag_list
) else if "%second_loader_input%" == "4" (
	goto second_cmd_uart1_pin_mux_list
) else if "%second_loader_input%" == "5" (
	goto second_console_baud_rate_list
) else if "%second_loader_input%" == "6" (
	goto second_cmd_baud_rate_list
) else if "%second_loader_input%" == "7" (
	goto second_cpu_frequency_list
) else if "%second_loader_input%" == "8" (
	goto Load_Config_Start
)
::#endregion


::#region 2nd Boot Loader Clock Case
rem ### 2nd boot loader Clock Case start ###
:second_clock_case_list
set /a second_clock_case_value=1
set second_clock_case_input=NULL
Set /a ad=0

@echo [HIMAX]
@echo [HIMAX] ----------------- 2nd Boot Loader Clock Case List ------------------- 
for /f "delims=" %%i in (%clock_case_path%) do (
	echo [HIMAX] !second_clock_case_value!. %%i
	set /a second_clock_case_value += 1
)
set /a clock_case_cnt = second_clock_case_value - 2

@echo [HIMAX]
set /p second_clock_case_input="[HIMAX] Please select options ( 1 ~ %clock_case_cnt% ): "

if not defined second_clock_case_input (
	@echo [HIMAX] Your selection can not be NULL please check and try again!
	goto second_clock_case_list
)

if %second_clock_case_input% leq %clock_case_cnt% (
	if %second_clock_case_input% gtr 0 (
		For /f "delims=" %%m in (%clock_case_path%) do (
			Set /a ad += 1
			if !ad! equ %second_clock_case_input% set second_clock_case_name=%%m
		)
	) else (
		@echo [HIMAX]
		@echo [HIMAX] Your selection %second_clock_case_input% is not on the list please check and try again!
		goto second_clock_case_list
	)
) else (
	@echo [HIMAX]
	@echo [HIMAX] Your selection %second_clock_case_input% is not on the list please check and try again!
	goto second_clock_case_list
)

@echo [HIMAX] Set 2nd boot loader clock case "%second_clock_case_name%" finished!
@echo [HIMAX]

if "%second_clock_case_input%" == "1" (
	Set "content_value_mask=00"
	goto set_second_clock_case_bin
) else if "%second_clock_case_input%" == "2" (
	Set "content_value_mask=01"
	goto set_second_clock_case_bin
) else if "%second_clock_case_input%" == "3" (
	Set "content_value_mask=02"
	goto set_second_clock_case_bin
) else if "%second_clock_case_input%" == "4" (
	Set "content_value_mask=03"
	goto set_second_clock_case_bin
) else if "%second_clock_case_input%" == "5" (
	Set "content_value_mask=04"
	goto set_second_clock_case_bin
) else if "%second_clock_case_input%" == "6" (
	Set "content_value_mask=05"
	goto set_second_clock_case_bin
) else if "%second_clock_case_input%" == "7" (
	Set "content_value_mask=06"
	goto set_second_clock_case_bin
) else if "%second_clock_case_input%" == "8" (
	Set "content_value_mask=07"
	goto set_second_clock_case_bin
) else if "%second_clock_case_input%" == "9" (
	Set "content_value_mask=08"
	goto set_second_clock_case_bin
) else if "%second_clock_case_input%" == "10" (
	Set "content_value_mask=09"
	goto set_second_clock_case_bin
) else if "%second_clock_case_input%" == "11" (
	Set "content_value_mask=0A"
	goto set_second_clock_case_bin
) else if "%second_clock_case_input%" == "12" (
	Set "content_value_mask=0B"
	goto set_second_clock_case_bin
) else if "%second_clock_case_input%" == "13" (
	goto 2nd_bootloader_selection
)
:set_second_clock_case_bin
certutil -f -encodehex common\cfg\Loader_config.bin Loader_config.txt >nul
del common\cfg\Loader_config.bin 
Set /a ae=0
set "content_value="
For /f "tokens=2-17" %%a in (Loader_config.txt) do (
	Set /a ae += 1
	if !ae! equ 3 (
		rem echo sam_shih %%a, %%b, %%c, %%d, !content_value_mask!, %%f, %%g, %%h, %%i, %%j, %%k, %%l, %%m, %%n, %%o, %%p
		set "content_value=%%a %%b %%c %%d !content_value_mask! %%f %%g %%h %%i %%j %%k %%l %%m %%n %%o %%p"
	) else (
		set "content_value=%%a %%b %%c %%d %%e %%f %%g %%h %%i %%j %%k %%l %%m %%n %%o %%p"
	)
	>>temp.txt echo(!content_value!
)
certutil -f -decodehex temp.txt common\cfg\Loader_config.bin >nul
del temp.txt
goto 2nd_bootloader_selection
rem ### 2nd boot loader Clock Case end ###
::#endregion


::#region 2nd Boot Loader Clock Type
rem ### 2nd boot loader Clock Type start ###
:second_clock_type_list
set /a second_clock_type_value=1
set second_clock_type_input=NULL
Set /a ad=0

@echo [HIMAX]
@echo [HIMAX] ----------------- 2nd Boot Loader Clock Type List ------------------- 
for /f "delims=" %%i in (%clock_type_path%) do (
	echo [HIMAX] !second_clock_type_value!. %%i
	set /a second_clock_type_value += 1
)
set /a clock_type_cnt = second_clock_type_value - 2

@echo [HIMAX]
set /p second_clock_type_input="[HIMAX] Please select options ( 1 ~ %clock_type_cnt% ): "

if not defined second_clock_type_input (
	@echo [HIMAX] Your selection can not be NULL please check and try again!
	goto second_clock_type_list
)

if %second_clock_type_input% leq %clock_type_cnt% (
	if %second_clock_type_input% gtr 0 (
		For /f "delims=" %%m in (%clock_type_path%) do (
			Set /a ad += 1
			if !ad! equ %second_clock_type_input% set second_clock_type_name=%%m
		)
	) else (
		@echo [HIMAX]
		@echo [HIMAX] Your selection %second_clock_type_input% is not on the list please check and try again!
		goto second_clock_type_list
	)
) else (
	@echo [HIMAX]
	@echo [HIMAX] Your selection %second_clock_type_input% is not on the list please check and try again!
	goto second_clock_type_list
)

@echo [HIMAX] Set 2nd boot loader clock type "%second_clock_type_name%" finished!
@echo [HIMAX]

if "%second_clock_type_input%" == "1" (
	set /a second_pll_disable_flag=1
	Set "content_value_mask=00"
	goto set_second_clock_type_bin
) else if "%second_clock_type_input%" == "2" (
	set /a second_pll_disable_flag=0
	Set "content_value_mask=01"
	goto set_second_clock_type_bin
) else if "%second_clock_type_input%" == "3" (
	goto 2nd_bootloader_selection
)
:set_second_clock_type_bin
certutil -f -encodehex common\cfg\Loader_config.bin Loader_config.txt >nul
del common\cfg\Loader_config.bin 
Set /a ae=0
set "content_value="
For /f "tokens=2-17" %%a in (Loader_config.txt) do (
	Set /a ae += 1
	if !ae! equ 3 (
		rem echo sam_shih %%a, %%b, %%c, %%d, %%e, !content_value_mask!, %%g, %%h, %%i, %%j, %%k, %%l, %%m, %%n, %%o, %%p
		set "content_value=%%a %%b %%c %%d %%e !content_value_mask! %%g %%h %%i %%j %%k %%l %%m %%n %%o %%p"
	) else (
		set "content_value=%%a %%b %%c %%d %%e %%f %%g %%h %%i %%j %%k %%l %%m %%n %%o %%p"
	)
	>>temp.txt echo(!content_value!
)
certutil -f -decodehex temp.txt common\cfg\Loader_config.bin >nul
del temp.txt
goto 2nd_bootloader_selection
rem ### 2nd boot loader Clock Case end ###
::#endregion


::#region 2nd Boot Loader Other Flag
rem ### 2nd boot loader Other Flag start ###
:second_other_flag_list
set /a second_other_flag_value=1
set second_other_flag_input=NULL
Set /a ad=0

@echo [HIMAX]
@echo [HIMAX] ----------------- 2nd Boot Loader Other Flag List ------------------- 
for /f "delims=" %%i in (%other_flag_path%) do (
	echo [HIMAX] !second_other_flag_value!. %%i
	set /a second_other_flag_value += 1
)
set /a other_flag_cnt = second_other_flag_value - 2

@echo [HIMAX]
set /p second_other_flag_input="[HIMAX] Please select options ( 1 ~ %other_flag_cnt% ): "

if not defined second_other_flag_input (
	@echo [HIMAX] Your selection can not be NULL please check and try again!
	goto second_other_flag_list
)

if %second_other_flag_input% leq %other_flag_cnt% (
	if %second_other_flag_input% gtr 0 (
		For /f "delims=" %%m in (%other_flag_path%) do (
			Set /a ad += 1
			if !ad! equ %second_other_flag_input% set second_other_flag_name=%%m
		)
	) else (
		@echo [HIMAX]
		@echo [HIMAX] Your selection %second_other_flag_input% is not on the list please check and try again!
		goto second_other_flag_list
	)
) else (
	@echo [HIMAX]
	@echo [HIMAX] Your selection %second_other_flag_input% is not on the list please check and try again!
	goto second_other_flag_list
)

@echo [HIMAX] Set 2nd boot loader other flag "%second_other_flag_name%" finished!
@echo [HIMAX]

if "%second_other_flag_input%" == "1" (
	Set "content_value_mask=00"
	goto set_second_other_flag_bin
) else if "%second_other_flag_input%" == "2" (
	Set "content_value_mask=01"
	goto set_second_other_flag_bin
) else if "%second_other_flag_input%" == "3" (
	Set "content_value_mask=02"
	goto set_second_other_flag_bin
) else if "%second_other_flag_input%" == "4" (
	goto 2nd_bootloader_selection
)
:set_second_other_flag_bin
certutil -f -encodehex common\cfg\Loader_config.bin Loader_config.txt >nul
del common\cfg\Loader_config.bin 
Set /a ae=0
set "content_value="
For /f "tokens=2-17" %%a in (Loader_config.txt) do (
	Set /a ae += 1
	if !ae! equ 3 (
		rem echo sam_shih %%a, %%b, %%c, %%d, %%e, %%f, !content_value_mask!, %%h, %%i, %%j, %%k, %%l, %%m, %%n, %%o, %%p
		set "content_value=%%a %%b %%c %%d %%e %%f !content_value_mask! %%h %%i %%j %%k %%l %%m %%n %%o %%p"
	) else (
		set "content_value=%%a %%b %%c %%d %%e %%f %%g %%h %%i %%j %%k %%l %%m %%n %%o %%p"
	)
	>>temp.txt echo(!content_value!
)
certutil -f -decodehex temp.txt common\cfg\Loader_config.bin >nul
del temp.txt
goto 2nd_bootloader_selection
rem ### 2nd boot loader Other Flag end ###
::#endregion


::#region 2nd Boot Loader Cmd UART1 Pin Mux
rem ### 2nd boot loader Cmd UART1 Pin Mux start ###
:second_cmd_uart1_pin_mux_list
set /a second_cmd_uart1_pin_mux_value=1
set second_uart1_pin_mux_input=NULL
Set /a ad=0

@echo [HIMAX]
@echo [HIMAX] ----------------- 2nd Boot Loader Cmd UART1 Pin Mux List ------------------- 
for /f "delims=" %%i in (%second_cmd_uart1_pin_mux_path%) do (
	echo [HIMAX] !second_cmd_uart1_pin_mux_value!. %%i
	set /a second_cmd_uart1_pin_mux_value += 1
)
set /a uart1_pin_mux_cnt = second_cmd_uart1_pin_mux_value - 2

@echo [HIMAX]
set /p second_uart1_pin_mux_input="[HIMAX] Please select options ( 1 ~ %uart1_pin_mux_cnt% ): "

if not defined second_uart1_pin_mux_input (
	@echo [HIMAX] Your selection can not be NULL please check and try again!
	goto second_cmd_uart1_pin_mux_list
)

if %second_uart1_pin_mux_input% leq %uart1_pin_mux_cnt% (
	if %second_uart1_pin_mux_input% gtr 0 (
		For /f "delims=" %%m in (%second_cmd_uart1_pin_mux_path%) do (
			Set /a ad += 1
			if !ad! equ %second_uart1_pin_mux_input% set second_uart1_pin_mux_name=%%m
		)
	) else (
		@echo [HIMAX]
		@echo [HIMAX] Your selection %second_uart1_pin_mux_input% is not on the list please check and try again!
		goto second_cmd_uart1_pin_mux_list
	)
) else (
	@echo [HIMAX]
	@echo [HIMAX] Your selection %second_uart1_pin_mux_input% is not on the list please check and try again!
	goto second_cmd_uart1_pin_mux_list
)

@echo [HIMAX] Set 2nd boot loader command uart1 pin mux "%second_uart1_pin_mux_name%" finished!
@echo [HIMAX]

if "%second_uart1_pin_mux_input%" == "1" (
	Set "content_value_mask=01"
	goto set_second_uart1_pin_mux_bin
) else if "%second_uart1_pin_mux_input%" == "2" (
	Set "content_value_mask=02"
	goto set_second_uart1_pin_mux_bin
) else if "%second_uart1_pin_mux_input%" == "3" (
	Set "content_value_mask=03"
	goto set_second_uart1_pin_mux_bin
) else if "%second_uart1_pin_mux_input%" == "4" (
	Set "content_value_mask=04"
	goto set_second_uart1_pin_mux_bin
) else if "%second_uart1_pin_mux_input%" == "4" (
	goto 2nd_bootloader_selection
)
:set_second_uart1_pin_mux_bin
certutil -f -encodehex common\cfg\Loader_config.bin Loader_config.txt >nul
del common\cfg\Loader_config.bin 
Set /a ae=0
set "content_value="
For /f "tokens=2-17" %%a in (Loader_config.txt) do (
	Set /a ae += 1
	if !ae! equ 3 (
		rem echo sam_shih %%a, %%b, %%c, %%d, %%e, %%f, %%g, !content_value_mask!, %%i, %%j, %%k, %%l, %%m, %%n, %%o, %%p
		set "content_value=%%a %%b %%c %%d %%e %%f %%g !content_value_mask! %%i %%j %%k %%l %%m %%n %%o %%p"
	) else (
		set "content_value=%%a %%b %%c %%d %%e %%f %%g %%h %%i %%j %%k %%l %%m %%n %%o %%p"
	)
	>>temp.txt echo(!content_value!
)
certutil -f -decodehex temp.txt common\cfg\Loader_config.bin >nul
del temp.txt
goto 2nd_bootloader_selection
rem ### 2nd boot loader Other Flag end ###
::#endregion


::#region 2nd Boot Loader Console Baud Rate
rem ### 2nd boot loader console baud rate start ###
:second_console_baud_rate_list
set second_console_baud_rate_input=NULL

@echo [HIMAX]
@echo [HIMAX] --------------- Console Baud Rate List ------------------ 
set /p second_console_baud_rate_input="[HIMAX] Please enter console baud rate (or press ENTER to use default: 115200): "

if "%second_console_baud_rate_input%" == "NULL" (
	set second_console_baud_rate_input=115200
	Set "content_value_mask=00"
	Set "content_value_mask_f=C2"
	Set "content_value_mask_n=01"
	Set "content_value_mask_t=00"
	goto set_second_console_baud_rate_bin
)

if %second_console_baud_rate_input% gtr 0 (
	if %second_console_baud_rate_input% lss 921601 (
		rem continue...
	) else (
		@echo [HIMAX]
		@echo [HIMAX] [ERROR] Your input "%second_console_baud_rate_input%" error, should be less than 921600
		@echo [HIMAX]
		goto second_console_baud_rate_list
	)
) else (
	@echo [HIMAX]
	@echo [HIMAX] [ERROR] Your input "%second_console_baud_rate_input%" error, should be greater than 0
	@echo [HIMAX]
	goto second_console_baud_rate_list
)
@echo [HIMAX]
set C=0123456789ABCDEF
set "var=%second_console_baud_rate_input%"
set str=
:hex_console_baud_colum
set /a tra=%var% %% 16
call,set tra=%%C:~%tra%,1%%
set /a var/=16
set str=%tra%%str%
if %var% geq 10 (
	goto hex_console_baud_colum
)
if %var% neq 0 (
	set hexstr=%var%%str%
) else (
	set hexstr=%str%
)
rem echo hexstr=%hexstr%

set /a length=0
set value_tmp=%hexstr%
:string_console_baud_length_counter
if defined value_tmp (
    set value_tmp=%value_tmp:~1%
    set /a length+=1
    goto string_console_baud_length_counter
)
set /a length=8-%length%
set /a ten_dec=10
set ret=1
for /L %%i in (1,1,%length%) do set /a ret*=%ten_dec%

set /a zero_complement=%ret%
set zero_complement=%zero_complement:~1%
set "result=%zero_complement%%hexstr%"
rem echo result=%result%

Set "content_value_mask=%result:~-2,2%"
Set "content_value_mask_f=%result:~-4,2%"
Set "content_value_mask_n=%result:~-6,2%"
Set "content_value_mask_t=%result:~-8,2%"
rem echo sam_shih-----!content_value_mask!, !content_value_mask_f!, !content_value_mask_n!, !content_value_mask_t!

:set_second_console_baud_rate_bin
@echo [HIMAX]
@echo [HIMAX] Set console baud rate "%second_console_baud_rate_input%" finished!
@echo [HIMAX]
certutil -f -encodehex common\cfg\Loader_config.bin Loader_config.txt >nul
del common\cfg\Loader_config.bin
set "content_value="
Set /a cf=0
For /f "tokens=2-17" %%a in (Loader_config.txt) do (
	Set /a cf += 1
	if !cf! equ 3 (
		rem echo sam_shihhh %%a, %%b, %%c, %%d, %%e, %%f, %%g, %%h, !content_value_mask!, !content_value_mask_f!, !content_value_mask_n!, !content_value_mask_t!, %%m, %%n, %%o, %%p
		set "content_value=%%a %%b %%c %%d %%e %%f %%g %%h !content_value_mask! !content_value_mask_f! !content_value_mask_n! !content_value_mask_t! %%m %%n %%o %%p"
	) else (
		rem echo sam_shihhh %%a, %%b, %%c, %%d, %%e, %%f, %%g, %%h, %%i, %%j, %%k, %%l, %%m, %%n, %%o, %%p
		set "content_value=%%a %%b %%c %%d %%e %%f %%g %%h %%i %%j %%k %%l %%m %%n %%o %%p"
	)
	>>temp.txt echo(!content_value!
) 
certutil -f -decodehex temp.txt common\cfg\Loader_config.bin >nul
del temp.txt
goto 2nd_bootloader_selection
rem ### 2nd boot loader console baud rate end ###
::#endregion


::#region 2nd Boot Loader Cmd Baud Rate
rem ### 2nd boot loader command baud rate start ###
:second_cmd_baud_rate_list
set second_cmd_baud_rate_input=NULL

@echo [HIMAX] --------------- OTA CMD Baud Rate List ------------------ 
set /p second_cmd_baud_rate_input="[HIMAX] Please enter OTA cmd baud rate (or press ENTER to use default: 921600): "

if "%second_cmd_baud_rate_input%" == "NULL" (
	set second_cmd_baud_rate_input=921600
	Set "content_value_mask=00"
	Set "content_value_mask_f=10"
	Set "content_value_mask_n=0E"
	Set "content_value_mask_t=00"
	goto set_second_cmd_baud_rate_bin
)

if %second_cmd_baud_rate_input% gtr 0 (
	if %second_cmd_baud_rate_input% lss 921601 (
		rem continue...
	) else (
		@echo [HIMAX]
		@echo [HIMAX] [ERROR] Your input "%second_cmd_baud_rate_input%" error, should be less than 921600
		@echo [HIMAX]
		goto second_cmd_baud_rate_list
	)
) else (
	@echo [HIMAX]
	@echo [HIMAX] [ERROR] Your input "%second_cmd_baud_rate_input%" error, should be greater than 0
	@echo [HIMAX]
	goto second_cmd_baud_rate_list
)
@echo [HIMAX]
set C=0123456789ABCDEF
set "var=%second_cmd_baud_rate_input%"
set str=
:hex_cmd_baud_colum
set /a tra=%var% %% 16
call,set tra=%%C:~%tra%,1%%
set /a var/=16
set str=%tra%%str%
if %var% geq 10 (
	goto hex_cmd_baud_colum
)
if %var% neq 0 (
	set hexstr=%var%%str%
) else (
	set hexstr=%str%
)
rem echo hexstr=%hexstr%

set /a length=0
set value_tmp=%hexstr%
:string_cmd_baud_length_counter
if defined value_tmp (
    set value_tmp=%value_tmp:~1%
    set /a length+=1
    goto string_cmd_baud_length_counter
)
set /a length=8-%length%
set /a ten_dec=10
set ret=1
for /L %%i in (1,1,%length%) do set /a ret*=%ten_dec%

set /a zero_complement=%ret%
set zero_complement=%zero_complement:~1%
set "result=%zero_complement%%hexstr%"
rem echo result=%result%

Set "content_value_mask=%result:~-2,2%"
Set "content_value_mask_f=%result:~-4,2%"
Set "content_value_mask_n=%result:~-6,2%"
Set "content_value_mask_t=%result:~-8,2%"
rem echo sam_shih-----!content_value_mask!, !content_value_mask_f!, !content_value_mask_n!, !content_value_mask_t!

:set_second_cmd_baud_rate_bin
@echo [HIMAX]
@echo [HIMAX] Set OTA command baud rate "%second_cmd_baud_rate_input%" finished!
@echo [HIMAX]
certutil -f -encodehex common\cfg\Loader_config.bin Loader_config.txt >nul
del common\cfg\Loader_config.bin
set "content_value="
Set /a cf=0
For /f "tokens=2-17" %%a in (Loader_config.txt) do (
	Set /a cf += 1
	if !cf! equ 3 (
		rem echo sam_shihhh %%a, %%b, %%c, %%d, %%e, %%f, %%g, %%h, %%i, %%j, %%k, %%l, !content_value_mask!, !content_value_mask_f!, !content_value_mask_n!, !content_value_mask_t!
		set "content_value=%%a %%b %%c %%d %%e %%f %%g %%h %%i %%j %%k %%l !content_value_mask! !content_value_mask_f! !content_value_mask_n! !content_value_mask_t!
	) else (
		rem echo sam_shihhh %%a, %%b, %%c, %%d, %%e, %%f, %%g, %%h, %%i, %%j, %%k, %%l, %%m, %%n, %%o, %%p
		set "content_value=%%a %%b %%c %%d %%e %%f %%g %%h %%i %%j %%k %%l %%m %%n %%o %%p"
	)
	>>temp.txt echo(!content_value!
)
certutil -f -decodehex temp.txt common\cfg\Loader_config.bin >nul
del temp.txt
goto 2nd_bootloader_selection
rem ### 2nd boot loader command baud rate end ###
::#endregion


::#region 2nd Boot Loader CPU Frequency
rem ### 2nd boot loader CPU Frequency start ###
:second_cpu_frequency_list
set /a second_cpu_frequency_value=1
set second_cpu_frequency_input=NULL
Set /a ad=0

@echo [HIMAX]
@echo [HIMAX] ----------------- 2nd Boot Loader CPU Frequency List ------------------- 
for /f "delims=" %%i in (%cpu_frequency_path%) do (
	echo [HIMAX] !second_cpu_frequency_value!. %%i
	set /a second_cpu_frequency_value += 1
)
set /a second_cpu_frequency_cnt = second_cpu_frequency_value - 2

@echo [HIMAX]
set /p second_cpu_frequency_input="[HIMAX] Please select options ( 1 ~ %second_cpu_frequency_cnt% ): "

if not defined second_cpu_frequency_input (
	@echo [HIMAX] Your selection can not be NULL please check and try again!
	goto second_cpu_frequency_list
)

if %second_cpu_frequency_input% leq %second_cpu_frequency_cnt% (
	if %second_cpu_frequency_input% gtr 0 (
		For /f "delims=" %%m in (%cpu_frequency_path%) do (
			Set /a ad += 1
			if !ad! equ %second_cpu_frequency_input% set second_cpu_frequency_name=%%m
		)
	) else (
		@echo [HIMAX]
		@echo [HIMAX] Your selection %second_cpu_frequency_input% is not on the list please check and try again!
		goto second_cpu_frequency_list
	)
) else (
	@echo [HIMAX]
	@echo [HIMAX] Your selection %second_cpu_frequency_input% is not on the list please check and try again!
	goto second_cpu_frequency_list
)

@echo [HIMAX] Set 2nd boot loader CPU frequency "%second_cpu_frequency_name%" finished!
@echo [HIMAX]

rem 100M
if "%second_cpu_frequency_input%" == "1" (
	Set "content_value_mask=00"
	Set "content_value_mask_f=E1"
	Set "content_value_mask_n=F5"
	Set "content_value_mask_t=05"
	goto set_second_cpu_frequency_bin
rem 150M
) else if "%second_cpu_frequency_input%" == "2" (
	Set "content_value_mask=80"
	Set "content_value_mask_f=D1"
	Set "content_value_mask_n=F0"
	Set "content_value_mask_t=08"
	goto set_second_cpu_frequency_bin
rem 200M
) else if "%second_cpu_frequency_input%" == "3" (
	Set "content_value_mask=00"
	Set "content_value_mask_f=C2"
	Set "content_value_mask_n=EB"
	Set "content_value_mask_t=0B"
	goto set_second_cpu_frequency_bin
rem 300M
) else if "%second_cpu_frequency_input%" == "4" (
	Set "content_value_mask=00"
	Set "content_value_mask_f=A3"
	Set "content_value_mask_n=E1"
	Set "content_value_mask_t=11"
	goto set_second_cpu_frequency_bin
rem 350M
) else if "%second_cpu_frequency_input%" == "5" (
	Set "content_value_mask=80"
	Set "content_value_mask_f=93"
	Set "content_value_mask_n=DC"
	Set "content_value_mask_t=14"
	goto set_second_cpu_frequency_bin
rem 400M
) else if "%second_cpu_frequency_input%" == "6" (
	Set "content_value_mask=00"
	Set "content_value_mask_f=84"
	Set "content_value_mask_n=D7"
	Set "content_value_mask_t=17"
	goto set_second_cpu_frequency_bin
rem 450M
) else if "%second_cpu_frequency_input%" == "7" (
	Set "content_value_mask=80"
	Set "content_value_mask_f=74"
	Set "content_value_mask_n=D2"
	Set "content_value_mask_t=1A"
	goto set_second_cpu_frequency_bin
) else if "%second_cpu_frequency_input%" == "8" (
	goto 2nd_bootloader_selection
)
:set_second_cpu_frequency_bin
certutil -f -encodehex common\cfg\Loader_config.bin Loader_config.txt >nul
del common\cfg\Loader_config.bin 
Set /a ae=0
set "content_value="
For /f "tokens=2-17" %%a in (Loader_config.txt) do (
	Set /a ae += 1
	if !ae! equ 4 (
		rem echo sam_shih !content_value_mask!, !content_value_mask_f!, !content_value_mask_n!, !content_value_mask_t!, %%e, %%f, %%g, %%h, %%i, %%j, %%k, %%l, %%m, %%n, %%o, %%p
		set "content_value=!content_value_mask! !content_value_mask_f! !content_value_mask_n! !content_value_mask_t! %%e %%f %%g %%h %%i %%j %%k %%l %%m %%n %%o %%p"
	) else (
		rem echo sam_shih %%a, %%b, %%c, %%d, %%e, %%f, %%g, %%h, %%i, %%j, %%k, %%l, %%m, %%n, %%o, %%p
		set "content_value=%%a %%b %%c %%d %%e %%f %%g %%h %%i %%j %%k %%l %%m %%n %%o %%p"
	)
	>>temp.txt echo(!content_value!
)
certutil -f -decodehex temp.txt common\cfg\Loader_config.bin >nul
del temp.txt
goto second_pll_setting_list
rem ### 2nd boot loader CPU Frequency end ###
::#endregion


::#region 2nd Boot Loader PLL Setting
rem ### 2nd boot loader pll_setting start ###
:second_pll_setting_list
for /L %%i in (1,1,%length%) do set /a ret*=%ten_dec%
for /L %%g in (1,1,24) do set pll_%%g_value=NULL

rem common values
Set "pll_3_value=61"
Set "pll_4_value=38"
Set "pll_5_value=8F"
Set "pll_6_value=83"
Set "pll_7_value=D1"
Set "pll_8_value=C0"
Set "pll_9_value=16"
Set "pll_14_value=D0"
Set "pll_16_value=C0"
Set "pll_19_value=00"
Set "pll_20_value=0A"
Set "pll_21_value=C0"
Set "pll_22_value=FE"
Set "pll_23_value=00"
Set "pll_24_value=00"

if %second_cpu_frequency_input% equ 1 (
	rem echo 1st cpu freq is 100MHz
	if %second_pll_disable_flag% equ 0 (
		rem default enable clock type
		Set "pll_1_value=43"
		Set "pll_2_value=28"
		Set "pll_10_value=32"
		Set "pll_11_value=90"
		Set "pll_12_value=04"
		Set "pll_13_value=00"
		Set "pll_15_value=73"
		Set "pll_17_value=08"
		Set "pll_18_value=00"
	) else (
		rem disable clock type
		Set "pll_1_value=03"
		Set "pll_2_value=28"
		Set "pll_10_value=32"
		Set "pll_11_value=80"
		Set "pll_12_value=10"
		Set "pll_13_value=00"
		Set "pll_15_value=CB"
		Set "pll_17_value=21"
		Set "pll_18_value=00"
	)
) else if %second_cpu_frequency_input% equ 2 (
	rem echo 1st cpu freq is 150MHz
	Set "pll_1_value=43"
	Set "pll_2_value=28"
	Set "pll_10_value=4B"
	Set "pll_11_value=90"
	Set "pll_12_value=06"
	Set "pll_13_value=00"
	Set "pll_15_value=AC"
	Set "pll_17_value=0C"
	Set "pll_18_value=00"
) else if %second_cpu_frequency_input% equ 3 (
	rem echo 1st cpu freq is 200MHz
	Set "pll_1_value=43"
	Set "pll_2_value=18"
	Set "pll_10_value=32"
	Set "pll_11_value=90"
	Set "pll_12_value=04"
	Set "pll_13_value=00"
	Set "pll_15_value=73"
	Set "pll_17_value=08"
	Set "pll_18_value=00"
) else if %second_cpu_frequency_input% equ 4 (
	rem echo 1st cpu freq is 300MHz
	Set "pll_1_value=43"
	Set "pll_2_value=18"
	Set "pll_10_value=4B"
	Set "pll_11_value=90"
	Set "pll_12_value=06"
	Set "pll_13_value=00"
	Set "pll_15_value=AC"
	Set "pll_17_value=0C"
	Set "pll_18_value=00"
) else if %second_cpu_frequency_input% equ 5 (
	rem echo 1st cpu freq is 350MHz
	Set "pll_1_value=43"
	Set "pll_2_value=18"
	Set "pll_10_value=58"
	Set "pll_11_value=90"
	Set "pll_12_value=07"
	Set "pll_13_value=0A"
	Set "pll_15_value=DE"
	Set "pll_17_value=0E"
	Set "pll_18_value=04"
) else if %second_cpu_frequency_input% equ 6 (
	rem echo 1st cpu freq is 400MHz
	Set "pll_1_value=43"
	Set "pll_2_value=10"
	Set "pll_10_value=32"
	Set "pll_11_value=90"
	Set "pll_12_value=04"
	Set "pll_13_value=00"
	Set "pll_15_value=73"
	Set "pll_17_value=08"
	Set "pll_18_value=00"
) else if %second_cpu_frequency_input% equ 7 (
	rem echo 1st cpu freq is 450MHz
	Set "pll_1_value=43"
	Set "pll_2_value=10"
	Set "pll_10_value=39"
	Set "pll_11_value=80"
	Set "pll_12_value=04"
	Set "pll_13_value=8F"
	Set "pll_15_value=A1"
	Set "pll_17_value=08"
	Set "pll_18_value=02"
)

certutil -f -encodehex common\cfg\Loader_config.bin Loader_config.txt >nul
del common\cfg\Loader_config.bin 
set "content_value="
Set /a cf=0
For /f "tokens=2-17" %%a in (Loader_config.txt) do (
	Set /a cf += 1
	if !cf! equ 4 (
		rem echo sam_shihhh %%a, %%b, %%c, %%d, !pll_1_value!, !pll_2_value!, !pll_3_value!, !pll_4_value!, !pll_5_value!, !pll_6_value!, !pll_7_value!, !pll_8_value!, !pll_9_value!, !pll_10_value!, !pll_11_value!, !pll_12_value!
		set "content_value=%%a %%b %%c %%d !pll_1_value! !pll_2_value! !pll_3_value! !pll_4_value! !pll_5_value! !pll_6_value! !pll_7_value! !pll_8_value! !pll_9_value! !pll_10_value! !pll_11_value! !pll_12_value!"
	) else if !cf! equ 5 (		
		rem echo sam_shihhh !pll_13_value!, !pll_14_value!, !pll_15_value!, !pll_16_value!, !pll_17_value!, !pll_18_value!, !pll_19_value!, !pll_20_value!, !pll_21_value!, !pll_22_value!, !pll_23_value!, !pll_24_value!, %%m, %%n, %%o, %%p
		set "content_value=!pll_13_value! !pll_14_value! !pll_15_value! !pll_16_value! !pll_17_value! !pll_18_value! !pll_19_value! !pll_20_value! !pll_21_value! !pll_22_value! !pll_23_value! !pll_24_value! %%m %%n %%o %%p"
	)  else (
		rem echo sam_shihhh %%a, %%b, %%c, %%d, %%e, %%f, %%g, %%h, %%i, %%j, %%k, %%l, %%m, %%n, %%o, %%p
		set "content_value=%%a %%b %%c %%d %%e %%f %%g %%h %%i %%j %%k %%l %%m %%n %%o %%p"
	)
	>>temp.txt echo(!content_value!
)
certutil -f -decodehex temp.txt common\cfg\Loader_config.bin >nul
del temp.txt
goto 2nd_bootloader_selection
rem ### 2nd boot loader pll_setting end ###
::#endregion
@echo [HIMAX]
rem ### loader config end ###
::#endregion


::#region Algo_Model_Start
:Algo_Model_Start
Setlocal EnableDelayedExpansion
set algo_model_path=ini_folder\algo_model.ini
@echo [HIMAX] ------------------- Algo Model List --------------------- 
set /a algo_model_cnt=1
set /a algo_model_tmp_cnt=1
set algo_model_input=NULL
Set /a az=0
for /f "delims=" %%i in (%algo_model_path%) do (
    echo [HIMAX] !algo_model_cnt!. %%i
	set /a algo_model_cnt += 1
)
set /a algo_model_tmp_cnt = algo_model_cnt - 2

@echo [HIMAX]
set /p algo_model_input="[HIMAX] Please select one option ( 1 ~ %algo_model_tmp_cnt% ): "

if not defined algo_model_input (
    @echo [HIMAX] Your selection can not be NULL please check and try again!
	goto Algo_Model_Start
)

if %algo_model_input% leq %algo_model_tmp_cnt% (
	if %algo_model_input% gtr 0 (
		For /f "delims=" %%c in (%algo_model_path%) do (
			Set /a az += 1
			rem echo [cdc] !az!. %%c
			if !az! equ %algo_model_input% set tmp_algo_model=%%c
		)
	) else (
		@echo [HIMAX]
		@echo [HIMAX] Your selection %algo_model_input% is not on the list please check and try again!
		goto Algo_Model_Start
	)
) else (
	@echo [HIMAX]
	@echo [HIMAX] Your selection %algo_model_input% is not on the list please check and try again!
	goto Algo_Model_Start
)

@echo [HIMAX] Your selection is "%tmp_algo_model%"
@echo [HIMAX]
if %algo_model_input% equ 1 (
	set /a amr_enable_flag=1
	goto AMR_Start
) else if %algo_model_input% equ 2 (
	set /a audio_enable_flag=1
	goto Audio_Start
) else if %algo_model_input% equ 3 (
	set /a fr_enable_flag=1
	goto FR_Start
) else if %algo_model_input% equ 4 (
	set /a standalone_enable_flag=1
	goto Standalone_Start
) else if %algo_model_input% equ 5 (
	goto menu_list
)


::#region AMR_Start
rem ### AMR option start ###
rem choice /C YN /M "[HIMAX] Setup AMR options: "
rem if %errorlevel% == 1 goto gen_amr_ini
rem if %errorlevel% == 2 goto gen_normal_ini
:AMR_Start
Setlocal EnableDelayedExpansion
@echo [HIMAX] ------------------- AMR Options List -------------------- 
set /p amr_cfg_input_file="[HIMAX] Please enter AMR configuration input file (or press ENTER to use default: input\wr_config.bin): "
if not defined amr_cfg_input_file (
    set amr_cfg_input_file=input\wr_config.bin
)
@echo [HIMAX]
if not exist %amr_cfg_input_file% (
	@echo [HIMAX] [WARNING] User input "%amr_cfg_input_file%" is not exist, using default path "input\wr_config.bin"
	@echo [HIMAX]
	set amr_cfg_input_file=input\wr_config.bin
)
set /p amr_cfg_flash_addr="[HIMAX] Please enter AMR configuration flash address (or press ENTER to use default: C0000): "
if not defined amr_cfg_flash_addr (
    set amr_cfg_flash_addr=C0000
)
@echo [HIMAX]
set /p amr_model_input_file="[HIMAX] Please enter AMR model input file (or press ENTER to use default: input\AMR.tflite): "
if not defined amr_model_input_file (
    set amr_model_input_file=input\AMR.tflite
)
@echo [HIMAX]
if not exist %amr_model_input_file% (
	@echo [HIMAX] [WARNING] User input "%amr_model_input_file%" is not exist, using default path "input\AMR.tflite"
	@echo [HIMAX]
	set amr_model_input_file=input\AMR.tflite
)
set /p amr_model_flash_addr="[HIMAX] Please enter AMR model flash address (or press ENTER to use default: D0000): "
if not defined amr_model_flash_addr (
    set amr_model_flash_addr=D0000
)
@echo [HIMAX]
:AMR_check_short_dial
set amr_use_short_dial_input=NULL
set /p amr_use_short_dial_input="[HIMAX] Do you want ot use short dial model? ( y/n ): "

if not defined amr_use_short_dial_input (
	@echo [HIMAX]
	@echo [HIMAX] Your selection can not be NULL please check and try again!
	goto AMR_check_short_dial
)

if "%amr_use_short_dial_input%" == "n" (
	@echo [HIMAX]
	goto Algo_Model_Start
) 
if "%amr_use_short_dial_input%" == "N" (
	@echo [HIMAX]
	goto Algo_Model_Start
) 
if "%amr_use_short_dial_input%" == "y" (
	goto AMR_set_short_dial
) 
if "%amr_use_short_dial_input%" == "Y" (
	goto AMR_set_short_dial
) 

@echo [HIMAX]
@echo [HIMAX] Your selection %amr_use_short_dial_input% is invalid please check and try again!
goto AMR_check_short_dial

:AMR_set_short_dial
@echo [HIMAX]
set /a amr_short_dial_enable_flag=1
set /p amr_short_dial_model_input_file="[HIMAX] Please enter AMR short dial model input file (or press ENTER to use default: input\AMR_DIAL.tflite): "
if not defined amr_short_dial_model_input_file (
    set amr_short_dial_model_input_file=input\AMR_DIAL.tflite
)
@echo [HIMAX]
if not exist %amr_short_dial_model_input_file% (
	@echo [HIMAX] [WARNING] User input "%amr_short_dial_model_input_file%" is not exist, using default path "input\AMR_DIAL.tflite"
	@echo [HIMAX]
	set amr_short_dial_model_input_file=input\AMR_DIAL.tflite
)
set /p amr_short_dial_model_flash_addr="[HIMAX] Please enter AMR short dial model flash address (or press ENTER to use default: 140000): "
if not defined amr_short_dial_model_flash_addr (
    set amr_short_dial_model_flash_addr=140000
)
goto Algo_Model_Start
::echo off > %cd%\%tmp_var%
rem 這裡可以是多個路徑，用空格間隔 
::for %%a in (%cd%) do (
	rem /r . 遞迴
	rem for /r . %%b in (*.txt) do (
	::for %%b in (*.txt) do (
		::if exist "%%b" ( 
			::type "%%b" | findstr /i "freertos" && echo %%b >> %cd%\%tmp_var%
		::)
	::) 
) 
::type %tmp_var% | find /i "test02.txt" && goto no 
::echo not found! 
::rem del fileList.txt>nul 2>nul echo. 
:::no 
rem cls echo. 
::echo record at %tmp_var%! 
::start %tmp_var%
rem ### AMR option end ###


@echo [HIMAX]
::#endregion


::#region Audio_Start
:Audio_Start
set audio_base_input_file=NULL
set audio_group1_input_file=NULL
set audio_group2_input_file=NULL
set audio_base_flash_addr=FA000
set audio_group1_flash_addr=124000
set audio_group2_flash_addr=125000
@echo [HIMAX] ------------------ Audio Options List ------------------- 
set /p audio_base_input_file="[HIMAX] Please enter Audio CYBase.mod input file path (or press ENTER to use default: input\CYBase.mod): "
if not defined audio_base_input_file (
    set audio_base_input_file=input\CYBase.mod
)
@echo [HIMAX]
if not exist %audio_base_input_file% (
	@echo [HIMAX] [WARNING] User input "%audio_base_input_file%" is not exist, using default path "input\CYBase.mod"
	@echo [HIMAX]
	set audio_base_input_file=input\CYBase.mod
)
set /p audio_base_flash_addr="[HIMAX] Please enter Audio CYBase.mod flash address (or press ENTER to use default: FA000): "
if not defined audio_base_flash_addr (
    set audio_base_flash_addr=FA000
)

:Audio_Group_Number
set audio_group_num=NULL
@echo [HIMAX]
set /p audio_group_num="[HIMAX] Please enter how many audio groups need to be inserted (1 or 2): "
if not defined audio_group_num (
    @echo [HIMAX] Audio groups input can not be NULL please check and try again!
	goto Audio_Group_Number
)

if %audio_group_num% leq 2 (
	if %audio_group_num% gtr 0 (
		@echo [HIMAX] Your Audio groups input is "%audio_group_num%"
		@echo [HIMAX]
	) else (
		@echo [HIMAX]
		@echo [HIMAX] Audio groups input can not be %audio_group_num% please check and try again!
		goto Audio_Group_Number
	)
) else (
	@echo [HIMAX]
	@echo [HIMAX] Audio groups input can not be %audio_group_num% please check and try again!
	goto Audio_Group_Number
)

set /p audio_group1_input_file="[HIMAX] Please enter Audio group 1 input file path (or press ENTER to use default: input\Group_1.mod): "
if not defined audio_group1_input_file (
    set audio_group1_input_file=input\Group_1.mod
)
@echo [HIMAX]
if not exist %audio_group1_input_file% (
	@echo [HIMAX] [WARNING] User input "%audio_group1_input_file%" is not exist, using default path "input\Group_1.mod"
	@echo [HIMAX]
	set audio_group1_input_file=input\Group_1.mod
)
set /p audio_group1_flash_addr="[HIMAX] Please enter Audio group 1 flash address (or press ENTER to use default: 124000): "
if not defined audio_group1_flash_addr (
    set audio_group1_flash_addr=124000
)
@echo [HIMAX]
if %audio_group_num% equ 2 (
	@echo [HIMAX]
	set /p audio_group2_input_file="[HIMAX] Please enter Audio group 2 input file path (or press ENTER to use default: input\Group_2.mod): "
	if not defined audio_group2_input_file (
    	set audio_group2_input_file=input\Group_2.mod
	)
	@echo [HIMAX]
	if not exist %audio_group2_input_file% (
		@echo [HIMAX] [WARNING] User input "%audio_group2_input_file%" is not exist, using default path "input\Group_2.mod"
		@echo [HIMAX]
		set audio_group2_input_file=input\Group_2.mod
	)
	set /p audio_group2_flash_addr="[HIMAX] Please enter Audio group 2 flash address (or press ENTER to use default: 125000): "
	if not defined audio_group2_flash_addr (
    	set audio_group2_flash_addr=125000
	)
)
@echo [HIMAX]
goto Algo_Model_Start
::#endregion


::#region Standalone_Start
rem ### Standalone option start ###
:Standalone_Start
Setlocal EnableDelayedExpansion
set standalone_ver_path=ini_folder\standalone_type.ini
@echo [HIMAX] ---------------- Standalone Options List ---------------- 
set /a standalone_ver_cnt=1
set /a standalone_ver_tmp_cnt=1
rem set standalone_ver_input=NULL
Set /a au=0
for /f "delims=" %%i in (%standalone_ver_path%) do (
    echo [HIMAX] !standalone_ver_cnt!. %%i
	set /a standalone_ver_cnt += 1
)
set /a standalone_ver_tmp_cnt = standalone_ver_cnt - 2

@echo [HIMAX]
set /p standalone_ver_input="[HIMAX] Please select one option ( 1 ~ %standalone_ver_tmp_cnt% ): "

if not defined standalone_ver_input (
	@echo [HIMAX]
    @echo [HIMAX] Your selection can not be NULL please check and try again!
	goto Standalone_Start
)

if %standalone_ver_input% leq %standalone_ver_tmp_cnt% (
	if %standalone_ver_input% gtr 0 (
		For /f "delims=" %%h in (%standalone_ver_path%) do (
			Set /a au += 1
			rem echo [cdc] !au!. %%c
			if !au! equ %standalone_ver_input% set tmp_standalone_ver=%%h
		)
	) else (
		@echo [HIMAX]
		@echo [HIMAX] Your selection %standalone_ver_input% is not on the list please check and try again!
		goto Standalone_Start
	)
) else (
	@echo [HIMAX]
	@echo [HIMAX] Your selection %standalone_ver_input% is not on the list please check and try again!
	goto Standalone_Start
)
@echo [HIMAX]
@echo [HIMAX] Your selection is "%tmp_standalone_ver%"
@echo [HIMAX]

if %standalone_ver_input% equ 1 (
	goto standalone_type_c_process
) else if %standalone_ver_input% equ 2 (
	goto standalone_type_d_process
)

:standalone_type_c_process
set /p standalone_c_input_file="[HIMAX] Please enter a folder with the same file name as *.elf and *.map files (or press ENTER to use default: input\standalone_template_algo\WEI_FW_mw_arcem9d_wei_r16): "
if not defined standalone_c_input_file (
    set standalone_c_input_file=input\standalone_template_algo\WEI_FW_mw_arcem9d_wei_r16
)
@echo [HIMAX]
if not exist %standalone_c_input_file% (
	@echo [HIMAX] [WARNING] User input "%standalone_c_input_file%" is not exist, using default path "input\standalone_template_algo\WEI_FW_mw_arcem9d_wei_r16"
	@echo [HIMAX]
	set standalone_c_input_file=input\standalone_template_algo\WEI_FW_mw_arcem9d_wei_r16
)
set /p standalone_c_flash_addr="[HIMAX] Please enter standalone type c flash address (or press ENTER to use default: 60000): "
if not defined standalone_c_flash_addr (
    set standalone_c_flash_addr=60000
)
goto menu_list

:standalone_type_d_process
set /p standalone_d_input_file="[HIMAX] Please enter standalone *.img file path (or press ENTER to use default: input\standalone_output.img): "
if not defined standalone_d_input_file (
    set standalone_d_input_file=input\standalone_output.img
)
@echo [HIMAX]
if not exist %standalone_d_input_file% (
	@echo [HIMAX] [WARNING] User input "%standalone_d_input_file%" is not exist, using default path "input\standalone_output.img"
	@echo [HIMAX]
	set standalone_d_input_file=input\standalone_output.img
)

set /p standalone_d_dsp_file="[HIMAX] Please enter standalone dsp *.bin file path (or press ENTER to use default: input\layout_standalone.bin): "
if not defined standalone_d_dsp_file (
    set standalone_d_dsp_file=input\layout_standalone.bin
)
@echo [HIMAX]
if not exist %standalone_d_dsp_file% (
	@echo [HIMAX] [WARNING] User input "%standalone_d_dsp_file%" is not exist, using default path "inputlayout_standalone.bin"
	@echo [HIMAX]
	set standalone_d_dsp_file=input\layout_standalone.bin
)

set /p standalone_d_flash_addr="[HIMAX] Please enter standalone type d flash address (or press ENTER to use default: 60000): "
if not defined standalone_d_flash_addr (
    set standalone_d_flash_addr=60000
)
@echo [HIMAX]
goto Algo_Model_Start
::#endregion


::#region FR_Start
:FR_Start
@echo [HIMAX] -------------------- FR Options List -------------------- 
set /p fr_cfg_input_file="[HIMAX] Please enter FR configuration input file (or press ENTER to use default: input\FR_Album_2K.bin): "
if not defined fr_cfg_input_file (
    set fr_cfg_input_file=input\FR_Album_2K.bin
)
@echo [HIMAX]
if not exist %fr_cfg_input_file% (
	@echo [HIMAX] [WARNING] User input "%fr_cfg_input_file%" is not exist, using default path "input\FR_Album_2K.bin"
	@echo [HIMAX]
)
set /p fr_cfg_flash_addr="[HIMAX] Please enter FR configuration flash address (or press ENTER to use default: 12f000): "
if not defined fr_cfg_flash_addr (
    set fr_cfg_flash_addr=12f000
)
@echo [HIMAX]
goto Algo_Model_Start
::#endregion

::#endregion


::#region User_Define_Start
:User_Define_Start
Setlocal EnableDelayedExpansion
@echo [HIMAX] ------------------- User Define List -------------------- 
@echo [HIMAX]
set /p user_define_input_file="[HIMAX] Please enter relative path of input file (example: input\tmp_config.bin): "
if not defined user_define_input_file (
	@echo [HIMAX]
    @echo [HIMAX] [ERROR] Your input "%user_define_input_file%" can not be NULL please check and try again!
	goto User_Define_Start
)
@echo [HIMAX]
if not exist %user_define_input_file% (
	@echo [HIMAX]
	@echo [HIMAX] [ERROR] User input "%user_define_input_file%" is not exist please check and try again!
	goto User_Define_Start
)
:User_Define_Addr
@echo [HIMAX]
set /p user_define_flash_addr="[HIMAX] Please enter hexadecimal flash address (example: 1A0000): "
if not defined user_define_flash_addr (
	@echo [HIMAX]
    @echo [HIMAX] [ERROR] Your input "%user_define_flash_addr%" flash address can not be NULL please check and try again!
	goto User_Define_Addr
)
set /a user_def_enable_flag=1
set /a user_input_cnt += 1
goto menu_list
::#endregion


::#region Dump_Sataus_Start
:Dump_Sataus_Start
type nul > input\Current_Loader_config.txt
certutil -f -encodehex input\Loader_config.bin input\Current_Loader_config.txt >nul

set /a za=0
set "first_BL_uart_var="
set "first_BL_re_encry_var="
set "first_BL_clk_source_var="
set "second_BL_ota_method_var="
set "second_BL_re_encry_var="
set "second_BL_clk_source_var="
For /f "tokens=2-17" %%a in (input\Current_Loader_config.txt) do (
	Set /a za += 1
	if !za! equ 1 (
		rem echo [ sam_shih Debug ] %%a, %%b, %%c, %%d, %%e, %%f, %%g, %%h, %%i, %%j, %%k, %%l, %%m, %%n, %%o, %%p
		
		if %%e equ 00 (set first_BL_uart_var=w/o print) else if %%e equ 01 (set first_BL_uart_var=w/i print log) else (set first_BL_uart_var=error value %%e)
		rem echo [ sam_shih Debug ] first_BL_uart_var: !first_BL_uart_var!
		
		if %%f equ 00 (set first_BL_re_encry_var=w/o re-encryption) else if %%f equ 01 (set first_BL_re_encry_var=w/i re-encryption) else (set first_BL_re_encry_var=error value %%f)
		rem echo [ sam_shih Debug ] first_BL_re_encry_var: !first_BL_re_encry_var!
		
		if %%g equ 00 (set first_BL_clk_source_var=RC36) else if %%g equ 01 (set first_BL_clk_source_var=XTAL24) else (set first_BL_clk_source_var=error value %%g)
		rem echo [ sam_shih Debug ] first_BL_clk_source_var: !first_BL_clk_source_var!
		
		set /A spi_freq_speed_val=0x%%l%%k%%j%%i
		rem echo [ sam_shih Debug ] spi_freq_speed_val=!spi_freq_speed_val!
		set /a spi_freq_speed_val /= 1000000
		rem echo [ sam_shih Debug ] spi_freq_speed_val=!spi_freq_speed_val!
		
		set /A cpu_freq_speed_val=0x%%p%%o%%n%%m
		rem echo [ sam_shih Debug ] cpu_freq_speed_val=!cpu_freq_speed_val!
		set /a cpu_freq_speed_val /= 1000000
		rem echo [ sam_shih Debug ] cpu_freq_speed_val=!cpu_freq_speed_val!
	)
)

@echo [HIMAX] --------------- Current Menuconfig Status --------------- 
@echo [HIMAX]
@echo [HIMAX] Chip Name:		"%tmp_chip_name%"
@echo [HIMAX] Secure Format:		"%sec_format_name%"
@echo [HIMAX] Max Flash Size:		"%ini_flash_size%"
@echo [HIMAX] Part Number:		"%part_number_name%"
if %amr_enable_flag% equ 1 (set amr_status_string=Enable) else (set amr_status_string=Disable)
@echo [HIMAX] AMR Option:		"%amr_status_string%"
if %standalone_enable_flag% equ 1 (set standalone_status_string=Enable) else (set standalone_status_string=Disable)
@echo [HIMAX] Standalone Option:	"%standalone_status_string%"
if %fr_enable_flag% equ 1 (set fr_status_string=Enable) else (set fr_status_string=Disable)
@echo [HIMAX] Face Recognition Option:	"%fr_status_string%"
if %audio_enable_flag% equ 1 (set audio_status_string=Enable) else (set audio_status_string=Disable)
@echo [HIMAX] Audio Option:	"%audio_status_string%"
@echo [HIMAX]
@echo [HIMAX] First Boot Loader: 
@echo [HIMAX] UART Print:		"%first_BL_uart_var%"
@echo [HIMAX] Re-encryption:		"%first_BL_re_encry_var%"
@echo [HIMAX] Clock Source:		"%first_BL_clk_source_var%"
@echo [HIMAX] SPI Frequency:		"%spi_freq_speed_val%MHz"
@echo [HIMAX] CPU Frequency:		"%cpu_freq_speed_val%MHz"
set /a za=0
For /f "tokens=2-17" %%a in (input\Current_Loader_config.txt) do (
	Set /a za += 1
	if !za! equ 2 (
		rem @echo [HIMAX] PLL Setting:		"00:0x%%a, 01:0x%%b, 02:0x%%c, 03:0x%%d, 04:0x%%e, 05:0x%%f, 06:0x%%g, 07:0x%%h, 08:0x%%i, 09:0x%%j, 0a:0x%%k, 0b:0x%%l, 0c:0x%%m, 0d:0x%%n, 0e:0x%%o, 0f:0x%%p
		set /p="[HIMAX] PLL Setting:		00:0x%%a, 01:0x%%b, 02:0x%%c, 03:0x%%d, 04:0x%%e, 05:0x%%f, 06:0x%%g, 07:0x%%h, 08:0x%%i, 09:0x%%j, 0a:0x%%k, 0b:0x%%l, 0c:0x%%m, 0d:0x%%n, 0e:0x%%o, 0f:0x%%p"<nul
	) else if !za! equ 3 (
		@echo 10:0x%%a, 11:0x%%b, 12:0x%%c, 13:0x%%d, 14:0x%%e, 15:0x%%f, 16:0x%%g, 17:0x%%h, 18:0x%%i, 19:0x%%j, 1a:0x%%k, 1b:0x%%l
		
		if %%m equ 00 (set second_BL_ota_method_var=I2C) else if %%m equ 01 (set second_BL_ota_method_var=UART) else (set second_BL_ota_method_var=error value %%m)
		
		if %%n equ 00 (set second_BL_re_encry_var=w/o re-encryption) else if %%n equ 01 (set second_BL_re_encry_var=w/i re-encryption) else (set second_BL_re_encry_var=error value %%n)
		
		if %%o equ 00 (set second_BL_clk_source_var=RC36) else if %%o equ 01 (set second_BL_clk_source_var=XTAL24) else (set second_BL_clk_source_var=error value %%o)
	) else if !za! equ 4 (
		set /A console_baud_rate_val=0x%%d%%c%%b%%a
		
		set /A command_baud_rate_val=0x%%h%%g%%f%%e
		
		set /A second_spi_freq_speed_val=0x%%l%%k%%j%%i
		set /a second_spi_freq_speed_val /= 1000000
		
		set /A second_cpu_freq_speed_val=0x%%p%%o%%n%%m
		set /a second_cpu_freq_speed_val /= 1000000
	)
)
@echo [HIMAX]
@echo [HIMAX] Second Boot Loader: 
@echo [HIMAX] OTA Method:		"%second_BL_ota_method_var%"
@echo [HIMAX] Re-encryption:		"%second_BL_re_encry_var%"
@echo [HIMAX] Clock Source:		"%second_BL_clk_source_var%"
@echo [HIMAX] Console Baud Rate:	"%console_baud_rate_val%"
if "%second_BL_ota_method_var%" == "UART" (
	@echo [HIMAX] Command Baud Rate:	"%command_baud_rate_val%"
)
@echo [HIMAX] SPI Frequency:		"%second_spi_freq_speed_val%MHz"
@echo [HIMAX] CPU Frequency:		"%second_cpu_freq_speed_val%MHz"
set /a za=0
For /f "tokens=2-17" %%a in (input\Current_Loader_config.txt) do (
	Set /a za += 1
	if !za! equ 5 (
		rem @echo [HIMAX] PLL Setting:		"00:0x%%a, 01:0x%%b, 02:0x%%c, 03:0x%%d, 04:0x%%e, 05:0x%%f, 06:0x%%g, 07:0x%%h, 08:0x%%i, 09:0x%%j, 0a:0x%%k, 0b:0x%%l, 0c:0x%%m, 0d:0x%%n, 0e:0x%%o, 0f:0x%%p
		set /p="[HIMAX] PLL Setting:		00:0x%%a, 01:0x%%b, 02:0x%%c, 03:0x%%d, 04:0x%%e, 05:0x%%f, 06:0x%%g, 07:0x%%h, 08:0x%%i, 09:0x%%j, 0a:0x%%k, 0b:0x%%l, 0c:0x%%m, 0d:0x%%n, 0e:0x%%o, 0f:0x%%p"<nul
	) else if !za! equ 6 (
		@echo 10:0x%%a, 11:0x%%b, 12:0x%%c, 13:0x%%d, 14:0x%%e, 15:0x%%f, 16:0x%%g, 17:0x%%h, 18:0x%%i, 19:0x%%j, 1a:0x%%k, 1b:0x%%l
	)
)
pause
del input\Current_Loader_config.txt
goto menu_list

::#endregion


::#region Generate_ini_File

:gen_amr_ini
(
	echo [PROJECT_BASE]
	echo flash_max_size = %ini_flash_size%
	echo oupt_file = output\output.img
	echo odm_wrapkey = odm_key\%tmp_chip_name%_%sec_format_name%\%part_number_name%\odm_wrapkey.key
	echo pubkey = odm_key\%tmp_chip_name%_%sec_format_name%\%part_number_name%\we1_root_rsa_key.der.pub
	echo prikey = odm_key\%tmp_chip_name%_%sec_format_name%\%part_number_name%\odm_rsa_key.der
	echo cert = odm_key\%tmp_chip_name%_%sec_format_name%\%part_number_name%\cert1_rsa.bin
	echo devision_size_limit = FB00  
	echo chip_name = %tmp_chip_name%
	echo part_number = %part_number_name%
	echo:
	echo [BOOTLOADER]
	echo pat_type = 0
	echo input_file = input\%sec_format_name%\%tmp_chip_name%\%part_number_name%\%ini_sign_forma%_PA8530_EM9D_Bootloader.bin
	echo sec_format = %sec_format_name%
	echo version = 2
	echo fw_type = 1
	echo flash_addr = 10000
	echo:
	echo [2ND_BOOTLOADER]
	echo pat_type = 1
	echo input_file = input\%sec_format_name%\%tmp_chip_name%\%part_number_name%\%ini_sign_forma%_PA8530_EM9D_2nd_Bootloader.bin
	echo sec_format = %sec_format_name%
	echo version = 2
	echo fw_type = 1
	echo flash_addr = 0
	echo:
	echo [MEMORY_DESCIRPTOR]
	echo pat_type = 3
	echo output_file = output\layout
	echo sec_format = BLp
	echo version = 2
	echo fw_type = 4
	echo flash_addr = 20000
	echo:
	echo [LOADER_CONFIG]
	echo pat_type = 12
	echo input_file = input\Loader_config.bin
	echo sec_format = BLp
	echo version = 2
	echo fw_type = 4
	echo flash_addr = 21000
	echo:
	echo [APPLICATION]
	echo pat_type = 4
	echo input_file = input\WEI_FW_mw_arcem9d_wei_r16
	echo sec_format = %sec_format_name%
	echo version = 2
	echo fw_type = 3
	echo flash_addr = 22000
	echo devision_size = FB00
	echo:
	echo [WATER_METER_CONFIG]
	echo pat_type = 10
	echo input_file = %amr_cfg_input_file%
	echo sec_format = RAW
	echo version = 2
	echo fw_type = 3
	echo flash_addr = %amr_cfg_flash_addr%
	echo:
	echo [WATER_METER_MODEL]
	echo pat_type = 11
	echo input_file = %amr_model_input_file%
	echo sec_format = RAW
	echo version = 2
	echo fw_type = 3
	echo flash_addr = %amr_model_flash_addr%
) > image_gen_config.ini

if "%amr_short_dial_enable_flag%" == "1" (
	(
		echo:
		echo [WATER_DIAL_COUNT]
		echo pat_type = 13
		echo input_file = %amr_short_dial_model_input_file%
		echo sec_format = RAW
		echo version = 2
		echo fw_type = 3
		echo flash_addr = %amr_short_dial_model_flash_addr%
	) >> image_gen_config.ini
)

if "%standalone_enable_flag%" == "1" (
	rem standalone type c (*.elf, *.map)
	if %standalone_ver_input% equ 1 (
		(
			echo:
			echo [STANDALONE]
			echo pat_type = c
			echo input_file = %standalone_c_input_file%
			echo sec_format = %sec_format_name%
			echo version = 2
			echo fw_type = 2
			echo flash_addr = %standalone_c_flash_addr%
			echo devision_size = FB00
		) >> image_gen_config.ini
	rem standalone type d (*.img, *.bin)
	) else if %standalone_ver_input% equ 2 (
		(
			echo:
			echo [STANDALONE]
			echo pat_type = d
			echo input_file = %standalone_d_input_file%
			echo standalone_dsp = %standalone_d_dsp_file%
			echo sec_format = RAW
			echo version = 2
			echo fw_type = 2
			echo flash_addr = %standalone_d_flash_addr%
			echo devision_size = FC80
		) >> image_gen_config.ini
	)
)

if "%fr_enable_flag%" == "1" (
	(
		echo:
		echo [FR_ALBUM]
		echo pat_type = b
		echo input_file = %fr_cfg_input_file%
		echo sec_format = RAW
		echo version = 2
		echo fw_type = 4
		echo flash_addr = %fr_cfg_flash_addr%
	) >> image_gen_config.ini
)

if "%audio_enable_flag%" == "1" (
	(
		echo:
		echo [AUDIO_MOD_BASE]
		echo pat_type = a
		echo input_file = %audio_base_input_file%
		echo sec_format = RAW
		echo version = 2
		echo fw_type = 4
		echo flash_addr = %audio_base_flash_addr%
		echo:
		echo [AUDIO_MOD_GROUP1]
		echo pat_type = a
		echo input_file = %audio_group1_input_file%
		echo sec_format = RAW
		echo version = 2
		echo fw_type = 4
		echo flash_addr = %audio_group1_flash_addr%
	) >> image_gen_config.ini
	if "%audio_group_num%" == "2" (
		(
			echo:
			echo [AUDIO_MOD_GROUP2]
			echo pat_type = a
			echo input_file = %audio_group2_input_file%
			echo sec_format = RAW
			echo version = 2
			echo fw_type = 4
			echo flash_addr = %audio_group2_flash_addr%
		) >> image_gen_config.ini
	)
)

if "%user_def_enable_flag%" == "1" (
	(
		echo:
		echo [USER_DEFINE%user_input_cnt%]
		echo pat_type = 70
		echo input_file = %user_define_input_file%
		echo sec_format = RAW
		echo version = 2
		echo fw_type = 4
		echo flash_addr = %user_define_flash_addr%
	) >> image_gen_config.ini
)


set /a amr_enable_flag=0
set /a fr_enable_flag=0
set /a user_def_enable_flag=0
set /a amr_short_dial_enable_flag=0
goto end_menuconfig_process

:gen_normal_ini
(
	echo [PROJECT_BASE]
	echo flash_max_size = %ini_flash_size%
	echo oupt_file = output\output.img
	echo odm_wrapkey = odm_key\%tmp_chip_name%_%sec_format_name%\%part_number_name%\odm_wrapkey.key
	echo pubkey = odm_key\%tmp_chip_name%_%sec_format_name%\%part_number_name%\we1_root_rsa_key.der.pub
	echo prikey = odm_key\%tmp_chip_name%_%sec_format_name%\%part_number_name%\odm_rsa_key.der
	echo cert = odm_key\%tmp_chip_name%_%sec_format_name%\%part_number_name%\cert1_rsa.bin
	echo devision_size_limit = FB00  
	echo chip_name = %tmp_chip_name%
	echo part_number = %part_number_name%
	echo:
	echo [BOOTLOADER]
	echo pat_type = 0
	echo input_file = input\%sec_format_name%\%tmp_chip_name%\%part_number_name%\%ini_sign_forma%_PA8530_EM9D_Bootloader.bin
	echo sec_format = %sec_format_name%
	echo version = 2
	echo fw_type = 1
	echo flash_addr = 10000
	echo:
	echo [2ND_BOOTLOADER]
	echo pat_type = 1
	echo input_file = input\%sec_format_name%\%tmp_chip_name%\%part_number_name%\%ini_sign_forma%_PA8530_EM9D_2nd_Bootloader.bin
	echo sec_format = %sec_format_name%
	echo version = 2
	echo fw_type = 1
	echo flash_addr = 0
	echo:
	echo [MEMORY_DESCIRPTOR]
	echo pat_type = 3
	echo output_file = output\layout
	echo sec_format = BLp
	echo version = 2
	echo fw_type = 4
	echo flash_addr = 20000
	echo:
	echo [LOADER_CONFIG]
	echo pat_type = 12
	echo input_file = input\Loader_config.bin
	echo sec_format = BLp
	echo version = 2
	echo fw_type = 4
	echo flash_addr = 21000
	echo:
	echo [APPLICATION]
	echo pat_type = 4
	echo input_file = input\WEI_FW_mw_arcem9d_wei_r16
	echo sec_format = %sec_format_name%
	echo version = 2
	echo fw_type = 3
	echo flash_addr = 22000
	echo devision_size = FB00
) > image_gen_config.ini

if "%standalone_enable_flag%" == "1" (
	rem standalone type c (*.elf, *.map)
	if %standalone_ver_input% equ 1 (
		(
			echo:
			echo [STANDALONE]
			echo pat_type = c
			echo input_file = %standalone_c_input_file%
			echo sec_format = %sec_format_name%
			echo version = 2
			echo fw_type = 2
			echo flash_addr = %standalone_c_flash_addr%
			echo devision_size = FB00
		) >> image_gen_config.ini
	rem standalone type d (*.img, *.bin)
	) else if %standalone_ver_input% equ 2 (
		(
			echo:
			echo [STANDALONE]
			echo pat_type = d
			echo input_file = %standalone_d_input_file%
			echo standalone_dsp = %standalone_d_dsp_file%
			echo sec_format = RAW
			echo version = 2
			echo fw_type = 2
			echo flash_addr = %standalone_d_flash_addr%
			echo devision_size = FC80
		) >> image_gen_config.ini
	)
)

if "%fr_enable_flag%" == "1" (
	(
		echo:
		echo [FR_ALBUM]
		echo pat_type = b
		echo input_file = %fr_cfg_input_file%
		echo sec_format = RAW
		echo version = 2
		echo fw_type = 4
		echo flash_addr = %fr_cfg_flash_addr%
	) >> image_gen_config.ini
)

if "%audio_enable_flag%" == "1" (
	(
		echo:
		echo [AUDIO_MOD_BASE]
		echo pat_type = a
		echo input_file = %audio_base_input_file%
		echo sec_format = RAW
		echo version = 2
		echo fw_type = 4
		echo flash_addr = %audio_base_flash_addr%
		echo:
		echo [AUDIO_MOD_GROUP1]
		echo pat_type = a
		echo input_file = %audio_group1_input_file%
		echo sec_format = RAW
		echo version = 2
		echo fw_type = 4
		echo flash_addr = %audio_group1_flash_addr%
	) >> image_gen_config.ini

	if "%audio_group_num%" == "2" (
		(
			echo:
			echo [AUDIO_MOD_GROUP2]
			echo pat_type = a
			echo input_file = %audio_group2_input_file%
			echo sec_format = RAW
			echo version = 2
			echo fw_type = 4
			echo flash_addr = %audio_group2_flash_addr%
		) >> image_gen_config.ini
	)
)

if "%user_def_enable_flag%" == "1" (
	:User_Define_Cnt
	if %user_input_cnt% geq 1 ( 
		(
			echo:
			echo [USER_DEFINE%user_input_cnt%]
			echo pat_type = 70
			echo input_file = %user_define_input_file%
			echo sec_format = RAW
			echo version = 2
			echo fw_type = 4
			echo flash_addr = %user_define_flash_addr%
		) >> image_gen_config.ini
		set /a user_input_cnt -= 1
		goto User_Define_Cnt
	)
)
goto end_menuconfig_process

::#endregion


:end_menuconfig_process
echo [HIMAX] ********************************************************** 
echo [HIMAX] ************************* SUCCESS ************************ 
echo [HIMAX] ********************************************************** 
echo [HIMAX] ********                                          ******** 
echo [HIMAX] ********      image_gen_config.ini generated      ******** 
echo [HIMAX] ********                                          ******** 
echo [HIMAX] ********************************************************** 
echo [HIMAX] ************************* SUCCESS ************************ 
echo [HIMAX] ********************************************************** 
echo [HIMAX]
exit /b 0

:fail_menuconfig_process
echo [HIMAX] ********************************************************** 
echo [HIMAX] ************************** FAIL ************************** 
echo [HIMAX] ********************************************************** 
echo [HIMAX] *******                                            ******* 
echo [HIMAX] *******    image_gen_config.ini generated fail!     ******* 
echo [HIMAX] *******                                            ******* 
echo [HIMAX] ********************************************************** 
echo [HIMAX] ************************** FAIL **************************
echo [HIMAX] ********************************************************** 
echo [HIMAX] 
exit /b 2