//
// Copyright (c) 2010-2011 Matthew Jack and Doug Binks
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.

//
// Notes:
//   - We use a single intermediate directory for compiled .obj files, which means
//     we don't support compiling multiple files with the same name. Could fix this
//     with either mangling names to include std::experimental::filesystem::paths,  or recreating folder structure
//
//
#ifndef PLATFORM_WINDOWS
#include "Compiler.h"

#include <string>
#include <vector>
#include <iostream>

#include <fstream>
#include <sstream>

#include "assert.h"
#include <sys/wait.h>

#include "ICompilerLogger.h"

using namespace std;
const char	completion_token[] = "_COMPLETION_TOKEN_" ;

class windows_compiler
{
public:
	windows_compiler()
		: is_compilation_complete( false )
        , logger( 0 )
        , m_ChildForCompilationPID( 0 )
	{
        m_PipeStdOut[0] = 0;
        m_PipeStdOut[1] = 1;
        m_PipeStdErr[0] = 0;
        m_PipeStdErr[1] = 1;
	}

	volatile bool		is_compilation_complete;
	generic_logger*	logger;
    pid_t               m_ChildForCompilationPID;
    int                 m_PipeStdOut[2];
    int                 m_PipeStdErr[2];
};

generic_compiler::generic_compiler() 
	: m_pImplData( 0 )
{
}

generic_compiler::~generic_compiler()
{
	delete m_pImplData;
}

std::string generic_compiler::get_object_file_extension() const
{
	return ".o";
}

bool generic_compiler::is_complete() const
{
    if( !m_pImplData->is_compilation_complete && m_pImplData->m_ChildForCompilationPID )
    {
        
        // check for whether process is closed
        int procStatus;
        pid_t ret = waitpid( m_pImplData->m_ChildForCompilationPID, &procStatus, WNOHANG);
        if( ret && ( WIFEXITED(procStatus) || WIFSIGNALED(procStatus) ) )
        {
            m_pImplData->is_compilation_complete = true;
            m_pImplData->m_ChildForCompilationPID = 0;
 
            // get output and log
            if( m_pImplData->logger )
            {
                const size_t buffSize = 256 * 80; //should allow for a few lines...
                char buffer[buffSize];
                ssize_t numread = 0;
                while( ( numread = read( m_pImplData->m_PipeStdOut[0], buffer, buffSize-1 ) ) > 0 )
                {
                    buffer[numread] = 0;
                    m_pImplData->logger->LogInfo( buffer );
                }
                
                while( ( numread = read( m_pImplData->m_PipeStdErr[0], buffer, buffSize-1 ) )> 0 )
                {
                    buffer[numread] = 0;
                    m_pImplData->logger->LogError( buffer );    //TODO: seperate warnings from errors.
                }
            }

            // close the pipes as this process no longer needs them.
            close( m_pImplData->m_PipeStdOut[0] );
            m_pImplData->m_PipeStdOut[0] = 0;
            close( m_pImplData->m_PipeStdErr[0] );
            m_pImplData->m_PipeStdErr[0] = 0;
        }
    }
	return m_pImplData->is_compilation_complete;
}

void generic_compiler::initialize( generic_logger * pLogger )
{
    m_pImplData = new windows_compiler;
    m_pImplData->logger = pLogger;
}

void generic_compiler::compile( const std::vector<std::experimental::filesystem::path>&	filesToCompile_,
			   const compiler_options&			compiler_options_,
			   std::vector<std::experimental::filesystem::path>		linkLibraryList_,
			    const std::experimental::filesystem::path&		moduleName_ )

{
    const std::vector<std::experimental::filesystem::path>& include_dir_list = compiler_options_.include_dir_list;
    const std::vector<std::experimental::filesystem::path>& library_dir_list = compiler_options_.library_dir_list;
    const char* pCompileOptions =  compiler_options_.compile_options.c_str();
    const char* pLinkOptions = compiler_options_.link_options.c_str();

    std::string compiler_location = compiler_options_.compiler_location.m_string;
    if (compiler_location.size()==0){
#ifdef __clang__
        compiler_location = "clang++ ";
#else // default to g++
        compiler_location = "g++ ";
#endif //__clang__
    }

    //NOTE: Currently doesn't check if a prior compile is ongoing or not, which could lead to memory leaks
	m_pImplData->is_compilation_complete = false;
    
    //create pipes
    if ( pipe( m_pImplData->m_PipeStdOut ) != 0 )
    {
        if( m_pImplData->logger )
        {
            m_pImplData->logger->LogError( "Error in Compiler::RunCompile, cannot create pipe - perhaps insufficient memory?\n");
        }
        return;
    }
    //create pipes
    if ( pipe( m_pImplData->m_PipeStdErr ) != 0 )
    {
        if( m_pImplData->logger )
        {
            m_pImplData->logger->LogError( "Error in Compiler::RunCompile, cannot create pipe - perhaps insufficient memory?\n");
        }
        return;
    }
    
    pid_t retPID;
    switch( retPID = fork() )
    {
        case -1: // error, no fork
            if( m_pImplData->logger )
            {
                m_pImplData->logger->LogError( "Error in Compiler::RunCompile, cannot fork() process - perhaps insufficient memory?\n");
            }
            return;
        case 0: // child process - carries on below.
            break;
        default: // current process - returns to allow application to run whilst compiling
            close( m_pImplData->m_PipeStdOut[1] );
            m_pImplData->m_PipeStdOut[1] = 0;
            close( m_pImplData->m_PipeStdErr[1] );
            m_pImplData->m_PipeStdErr[1] = 0;
            m_pImplData->m_ChildForCompilationPID = retPID;
           return;
    }
    
    //duplicate the pipe to stdout, so output goes to pipe
    dup2( m_pImplData->m_PipeStdErr[1], STDERR_FILENO );
    dup2( m_pImplData->m_PipeStdOut[1], STDOUT_FILENO );
    close( m_pImplData->m_PipeStdOut[0] );
    m_pImplData->m_PipeStdOut[0] = 0;
    close( m_pImplData->m_PipeStdErr[0] );
    m_pImplData->m_PipeStdErr[0] = 0;

	std::string compileString = compiler_location + " " + "-g -fPIC -fvisibility=hidden -shared ";

#ifndef __LP64__
	compileString += "-m32 ";
#endif

	optimization_levels optimizationLevel = get_actual_optimization_level( compiler_options_.optimizationLevel );
	switch( optimizationLevel )
	{
	case optimization_level_default:
		assert(false);
	case optimization_level_debug:
		compileString += "-O0 ";
		break;
	case optimization_level_release:
		compileString += "-Os ";
		break;
	case optimization_level_unset:;
	}
    
	// Check for intermediate directory, create it if required
	// There are a lot more checks and robustness that could be added here
	if( !compiler_options_.intermediate_std::experimental::filesystem::path.Exists() )
	{
		bool success = compiler_options_.intermediate_std::experimental::filesystem::path.CreateDir();
		if( success && m_pImplData->logger ) { m_pImplData->logger->LogInfo("Created intermediate folder \"%s\"\n", compiler_options_.intermediate_std::experimental::filesystem::path.c_str()); }
		else if( m_pImplData->logger ) { m_pImplData->logger->LogError("Error creating intermediate folder \"%s\"\n", compiler_options_.intermediate_std::experimental::filesystem::path.c_str()); }
	}

	std::experimental::filesystem::path	output = moduleName_;
	bool bCopyOutput = false;
	if( compiler_options_.intermediate_std::experimental::filesystem::path.Exists() )
	{
		// add save object files
        compileString = "cd \"" + compiler_options_.intermediate_std::experimental::filesystem::path.m_string + "\"\n" + compileString + " --save-temps ";
		output = compiler_options_.intermediate_std::experimental::filesystem::path / "a.out";
		bCopyOutput = true;
	}
	
	
    // include directories
    for( size_t i = 0; i < include_dir_list.size(); ++i )
	{
        compileString += "-I\"" + include_dir_list[i].m_string + "\" ";
    }
    
    // library and framework directories
    for( size_t i = 0; i < library_dir_list.size(); ++i )
	{
        compileString += "-L\"" + library_dir_list[i].m_string + "\" ";
        compileString += "-F\"" + library_dir_list[i].m_string + "\" ";
    }
    
	if( !bCopyOutput )
	{
	    // output file
	    compileString += "-o " + output.m_string + " ";
	}


	if( pCompileOptions )
	{
		compileString += pCompileOptions;
		compileString += " ";
	}
	if( pLinkOptions && strlen(pLinkOptions) )
	{
		compileString += "-Wl,";
		compileString += pLinkOptions;
		compileString += " ";
	}
	
    // files to compile
    for( size_t i = 0; i < filesToCompile_.size(); ++i )
    {
        compileString += "\"" + filesToCompile_[i].m_string + "\" ";
    }
    
    // libraries to link
    for( size_t i = 0; i < linkLibraryList_.size(); ++i )
    {
        compileString += " " + linkLibraryList_[i].m_string + " ";
    }

	if( bCopyOutput )
	{
        compileString += "\n mv \"" + output.m_string + "\" \"" + moduleName_.m_string + "\"\n";
	}

    
    std::cout << compileString << std::endl << std::endl;

    execl("/bin/sh", "sh", "-c", compileString.c_str(), (const char*)NULL);
}
#endif