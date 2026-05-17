# custom action before building SPIFFS image. For example, compress HTML, etc.
# SCRIPT TO GZIP CRITICAL FILES FOR ACCELERATED WEBSERVING

# see also https://community.platformio.org/t/question-esp32-compress-files-in-data-to-gzip-before-upload-possible-to-spiffs/6274/10

#

# Extensively modified. See original at the link cited above.

Import( 'env', 'projenv' )

import os

import gzip

import shutil

import glob

# Complementary function for using gzip

def gzip_file( src_path, dst_path ):

    with open( src_path, 'rb' ) as src, gzip.open( dst_path, 'wb' ) as dst:

        for chunk in iter( lambda: src.read(4096), b"" ):

            dst.write( chunk )

# Compresses the defined files from 'data_src/' into 'data/'

def gzip_webfiles( source, target, env ):

    

    # Types of files that need to be compressed

    filetypes_to_gzip = [ 'css', 'html', 'js', 'ico' ]

    print( '\nGZIP: Starting the gzipping process for the LittleFS image...\n' )

    

    data_src_dir_path = os.path.join(env.get('PROJECT_DIR'), 'data_src')

    data_dir_path = env.get( 'PROJECT_DATA_DIR' )

    

    # Check if `data` and `datasrc` exist. If the first one exists but the second one doesn't, rename it.


    if(os.path.exists(data_dir_path) and not os.path.exists(data_src_dir_path) ):

        print('GZIP: The directory"'+data_dir_path+'" exist, "'+data_src_dir_path+'" not found.')

        print('GZIP: Renaming "' + data_dir_path + '" a "' + data_src_dir_path + '"')

        os.rename(data_dir_path, data_src_dir_path)

    # Remove the 'data' directory

    if(os.path.exists(data_dir_path)):

        print('GZIP: Deleting the "data" directory ' + data_dir_path)

        shutil.rmtree(data_dir_path)

    # Recreate the empty 'data' directory

    print('GZIP: Re-creating an empty data directory ' + data_dir_path)

    os.mkdir(data_dir_path)

    # I determine which files to compress

    files_to_gzip = []

    for extension in filetypes_to_gzip:        

        files_to_gzip.extend( glob.glob( os.path.join( data_src_dir_path, '*.' + extension ) ) )

    

    all_files = glob.glob(os.path.join(data_src_dir_path, '*.*'))

    files_to_copy = list(set(all_files) - set(files_to_gzip))

    #print('MEWGZIP: files to copy: ' + str(files_to_gzip))

    for file in files_to_copy:

        print('GZIP: Copying file:' + file + ' to data directory')

        shutil.copy(file, data_dir_path)

    # Compress and move files

    #print('MEWGZIP: files to compress: ' + str(files_to_gzip))

    was_error = False

    try:

        for source_file_path in files_to_gzip:

            print( 'GZIP: Comprimiendo... ' + source_file_path )

            #base_file_path = source_file_path.replace( source_file_prefix, '' )

            base_file_path = source_file_path

            target_file_path = os.path.join( data_dir_path, os.path.basename( base_file_path ) + '.gz' )

            

            # print( 'GZIP: GZIPPING FILE...\n' + source_file_path + ' TO...\n' + target_file_path + "\n\n" )

            print( 'GZIP: Compressed...' + target_file_path )

            gzip_file( source_file_path, target_file_path )

    except IOError as e:

        was_error = True

        print( 'GZIP: File compression failed: ' + source_file_path )

        # print( 'GZIP: EXCEPTION... {}'.format( e ) )

    if was_error:

        print( 'GZIP: Error/Incompleten' )

    else:

        print( 'GZIP: Compressed correctly.\n' )

# IMPORTANT, this needs to be added to call the routine

env.AddPreAction( '$BUILD_DIR/littlefs.bin', gzip_webfiles )