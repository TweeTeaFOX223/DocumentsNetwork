@echo off
REM setup.bat - 初期セットアップ

echo Setting up portable Apache environment...

@REM ディレクトリのパスを取得(バックスラッシュを修正する)
for /f "delims=" %%i in ('powershell -Command "$pwd.Path.Replace('\', '/') + '/'"') do set CURR_DIR=%%i
echo %CURR_DIR%


REM Apache ZIP パッケージのダウンロードと展開
if not exist Apache24\bin\httpd.exe (
    echo Downloading Apache...
    
    @REM curl を使ってApacheをダウンロード（リダイレクトに対応）
    curl -L -o apache.zip https://www.apachelounge.com/download/VS18/binaries/httpd-2.4.66-260223-Win64-VS18.zip
    
    if not exist apache.zip (
        echo Download failed. Please download manually.
        goto manual_instructions
    )
    
    echo Extracting Apache...
    
    @REM tarコマンドを使って展開
    mkdir apache_temp
    tar -xf apache.zip -C apache_temp
    
    @REM 展開したApache24フォルダをコピー
    xcopy /E /I apache_temp\Apache24 Apache24
    
    @REM 一時ファイルを削除
    rmdir /S /Q apache_temp
    del apache.zip
    
    if not exist Apache24\bin\httpd.exe (
        echo Extraction failed!
        goto manual_instructions
    )


    @REM Apache設定ファイルの調整--------------------------
    echo Configuring Apache...

    @REM サーバーのルートディレクトリを設定
    powershell -Command "(Get-Content Apache24\conf\httpd.conf) -replace 'c:/Apache24', '%CURR_DIR%Apache24' | Set-Content Apache24\conf\httpd.conf"
    
    @REM cgi_moduleを有効
    powershell -Command "(Get-Content Apache24\conf\httpd.conf) -replace '#LoadModule cgi_module', 'LoadModule cgi_module' | Set-Content Apache24\conf\httpd.conf"

    @REM CGI設定を追加(cgi-binのディレクトリにcgiファイルを配置)
    echo ^<Directory "%CURR_DIR%Apache24\cgi-bin"^> >> Apache24\conf\httpd.conf
    echo     Options +ExecCGI >> Apache24\conf\httpd.conf
    echo     AddHandler cgi-script .cgi .exe >> Apache24\conf\httpd.conf
    echo     Require all granted >> Apache24\conf\httpd.conf
    echo ^</Directory^> >> Apache24\conf\httpd.conf
    echo ScriptAlias /cgi-bin/ "%CURR_DIR%Apache24\cgi-bin\" >> Apache24\conf\httpd.conf

    @REM テスト用のCGIを配置
    (
    echo @echo off
    echo echo Content-Type: text/plain
    echo echo.
    echo echo CGI Test Successful
    ) > %CURR_DIR%Apache24\cgi-bin\test.bat


    @REM CGIをコンパイル(GCCのインストール必須)
    gcc c_file\main.c c_file\copyFile.c c_file\mkwid.c ^
    c_file\mklbl.c c_file\nnsk5.c c_file\mst.c  c_file\hml.c  ^
    c_file\knn.c c_file\se.c  c_file\mds.c  c_file\ce.c ^
    c_file\kk.c  c_file\cJSON.c  c_file\preprocess.c  ^
    c_file\generateGraph.c c_file\generateNetwork.c ^
    -o main.cgi -lm

    @REM コンパイルしたCGIをApacheのサーバーに移動
    move main.cgi Apache24\cgi-bin\

    @REM Apacheのサーバーのcgi-binにdataとresultのディレクトリを作成
    @REM 重要：C言語のプログラムの仕様上、mkdirしないとエラーになります
    mkdir Apache24\cgi-bin\data
    mkdir Apache24\cgi-bin\result

    @REM dataディレクトリにあるドキュメントをApacheのサーバーにコピー
    copy ..\0_data\ Apache24\cgi-bin\data

    echo Setup completed successfully!
    pause
    goto :eof

    :manual_instructions
    echo Please follow these steps manually:
    echo 1. Visit https://www.apachelounge.com/download/
    echo 2. Download the latest Win64 zip package
    echo 3. Extract the Apache24 folder to this directory and rename it to 'Apache24'
    echo 4. Run this script again after extraction
)


pause