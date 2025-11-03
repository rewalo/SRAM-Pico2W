@echo off
setlocal EnableExtensions

echo ===========================================================================
echo   Building SRAM-Pico2W Technical Documentation PDF
echo ===========================================================================
echo.

set "ROOT=%~dp0.."
set "SCRIPTS_DIR=%ROOT%\scripts"
set "DOCS_DIR=%ROOT%\docs"
set "TEX_FILE=%SCRIPTS_DIR%\technical_documentation.tex"
set "PDF_FILE=%DOCS_DIR%\technical_documentation.pdf"

if not exist "%TEX_FILE%" (
    echo [ERROR] technical_documentation.tex not found!
    echo Expected location: %TEX_FILE%
    pause
    exit /b 1
)

if not exist "%DOCS_DIR%" (
    echo [ERROR] docs directory not found!
    echo Expected location: %DOCS_DIR%
    pause
    exit /b 1
)

echo [INFO] Compiling LaTeX document...
echo.

:: Check if Python is available
where python >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Python is not installed or not in PATH!
    echo.
    echo Please download and install Python:
    echo   - Python 3: https://www.python.org/downloads/
    echo.
    echo After installation:
    echo   1. Make sure to check "Add Python to PATH" during installation
    echo   2. Restart your terminal/command prompt
    echo   3. Run this script again
    echo.
    pause
    exit /b 1
)

echo [OK] Python found
echo.

:: Check if pdflatex is available
where pdflatex >nul 2>&1
if errorlevel 1 (
    echo [ERROR] pdflatex not found in PATH!
    echo.
    echo Please download Python:
    echo   - Python 3: https://www.python.org/downloads/
    echo.
    echo After installation:
    echo   1. Make sure to check "Add Python to PATH" during installation
    echo   2. Restart your terminal/command prompt
    echo   3. Run this script again
    echo.
    pause
    exit /b 1
)

echo [OK] pdflatex found
echo.

:: Compile LaTeX (two passes: first for references/TOC, second to resolve)
echo [STEP 1/2] First compilation pass (generates TOC and references)...
cd /d "%SCRIPTS_DIR%"
pdflatex -interaction=nonstopmode -output-directory="%DOCS_DIR%" "technical_documentation.tex" >nul 2>&1
set FIRST_PASS_ERROR=%ERRORLEVEL%

echo [STEP 2/2] Second compilation pass (resolves TOC and cross-references)...
cd /d "%SCRIPTS_DIR%"
pdflatex -interaction=nonstopmode -output-directory="%DOCS_DIR%" "technical_documentation.tex" >nul 2>&1
set SECOND_PASS_ERROR=%ERRORLEVEL%

if not exist "%PDF_FILE%" (
    echo [ERROR] PDF file was not generated!
    echo.
    echo Trying verbose compilation to see errors...
    cd /d "%SCRIPTS_DIR%"
    pdflatex -interaction=errorstopmode -output-directory="%DOCS_DIR%" "technical_documentation.tex"
    pause
    exit /b 1
)

echo [INFO] Cleaning up temporary files...
if exist "%DOCS_DIR%\*.aux" del "%DOCS_DIR%\*.aux" >nul 2>&1
if exist "%DOCS_DIR%\*.log" del "%DOCS_DIR%\*.log" >nul 2>&1
if exist "%DOCS_DIR%\*.out" del "%DOCS_DIR%\*.out" >nul 2>&1
if exist "%DOCS_DIR%\*.toc" del "%DOCS_DIR%\*.toc" >nul 2>&1

if exist "%PDF_FILE%" (
    echo.
    echo ===========================================================================
    echo [SUCCESS] Documentation built successfully!
    echo ===========================================================================
    echo Output file: technical_documentation.pdf
    echo Location: %DOCS_DIR%
    echo.
    echo Note: Some warnings ^(overfull hbox, MiKTeX updates^) are normal and can
    echo       be ignored. The PDF was generated successfully.
    echo.
) else (
    echo [ERROR] PDF file not generated!
    pause
    exit /b 1
)

pause
exit /b 0