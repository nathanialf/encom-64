pipeline {
    agent any
    
    environment {
        // Use aws-encom-dev credentials for AWS operations
        AWS_PROFILE = 'encom-dev'
        N64_TOOLCHAIN = "${WORKSPACE}/tools/libdragon"
        PATH = "${N64_TOOLCHAIN}/bin:${PATH}"
    }
    
    stages {
        stage('Setup Toolchain') {
            steps {
                script {
                    // Check if libdragon toolchain exists, if not download and setup
                    if (!fileExists('tools/libdragon')) {
                        echo 'Setting up libdragon toolchain...'
                        sh '''
                            mkdir -p tools
                            cd tools
                            
                            # Download pre-built libdragon toolchain (.deb package)
                            echo "Downloading pre-built libdragon toolchain..."
                            wget -O gcc-toolchain-mips64.deb \
                                "https://github.com/DragonMinded/libdragon/releases/download/toolchain-continuous-prerelease/gcc-toolchain-mips64-x86_64.deb"
                            
                            # Extract .deb package without installing (portable approach)
                            echo "Extracting toolchain..."
                            ar x gcc-toolchain-mips64.deb
                            tar -xzf data.tar.gz
                            
                            # Move extracted files to libdragon directory
                            mv opt/libdragon libdragon
                            
                            # Clean up extraction files
                            rm -f gcc-toolchain-mips64.deb control.tar.* data.tar.* debian-binary
                            rm -rf opt
                            
                            echo "Libdragon toolchain setup complete"
                            ls -la libdragon/bin/ | head -5
                        '''
                    } else {
                        echo 'Libdragon toolchain already exists, skipping setup'
                    }
                    
                    // Setup libdragon source and build libraries + tools
                    if (!fileExists('tools/libdragon-src')) {
                        echo 'Setting up libdragon source and building libraries...'
                        sh '''
                            cd tools
                            
                            # Clone libdragon source for headers and libraries
                            echo "Cloning libdragon source..."
                            git clone https://github.com/DragonMinded/libdragon.git libdragon-src
                            
                            # Build libdragon libraries
                            echo "Building libdragon libraries..."
                            cd libdragon-src
                            N64_INST=${PWD}/../libdragon make clean
                            N64_INST=${PWD}/../libdragon make -j4
                            
                            # Build n64tool for ROM creation
                            echo "Building n64tool..."
                            cd tools
                            make n64tool
                            
                            echo "Libdragon setup complete"
                        '''
                    } else {
                        echo 'Libdragon source already exists, skipping setup'
                    }
                }
            }
        }
        
        stage('Fetch Map Data') {
            steps {
                echo 'Fetching map data from ENCOM API...'
                withCredentials([
                    aws(credentialsId: 'aws-encom-dev', accessKeyVariable: 'AWS_ACCESS_KEY_ID', secretKeyVariable: 'AWS_SECRET_ACCESS_KEY')
                ]) {
                    sh '''
                        # Create generated source directory
                        mkdir -p src/generated
                        
                        # Fetch map data from dev API with small map (10 hexagons)
                        echo "Fetching map data from API..."
                        curl -X POST https://encom-api-dev.riperoni.com/api/v1/map/generate \
                            -H "Content-Type: application/json" \
                            -d '{"hexagonCount": 10}' \
                            -o map_response.json
                        
                        # Verify the response is valid JSON
                        python3 -m json.tool map_response.json > /dev/null
                        
                        echo "Map data fetched successfully"
                    '''
                }
            }
        }
        
        stage('Generate Map Headers') {
            steps {
                echo 'Converting JSON map data to C headers...'
                sh '''
                    # Run the map converter (we'll create this script)
                    python3 scripts/map_converter.py map_response.json src/generated/map_data.h
                    
                    echo "Generated C header files:"
                    ls -la src/generated/
                '''
            }
        }
        
        stage('Build ROM') {
            steps {
                echo 'Building N64 ROM...'
                sh '''
                    # Set up environment for libdragon
                    export N64_INST=${PWD}/tools/libdragon
                    export PATH=${N64_INST}/bin:${PATH}
                    
                    # Build the ROM
                    make clean
                    make
                    
                    # Verify ROM was created
                    if [ -f "encom-64.z64" ]; then
                        echo "ROM built successfully: $(ls -lh encom-64.z64)"
                    else
                        echo "ERROR: ROM build failed"
                        exit 1
                    fi
                '''
            }
        }
        
        stage('Run Tests') {
            steps {
                echo 'Running unit tests...'
                sh '''
                    # Run any unit tests we create
                    # For now, just verify the ROM file integrity
                    
                    # Check ROM size (should be reasonable)
                    ROM_SIZE=$(stat -c%s "encom-64.z64")
                    echo "ROM size: ${ROM_SIZE} bytes"
                    
                    if [ ${ROM_SIZE} -gt 67108864 ]; then  # 64MB limit
                        echo "ERROR: ROM too large (>${ROM_SIZE} bytes)"
                        exit 1
                    fi
                    
                    if [ ${ROM_SIZE} -lt 1024 ]; then  # Should be at least 1KB
                        echo "ERROR: ROM too small (${ROM_SIZE} bytes)"
                        exit 1
                    fi
                    
                    echo "ROM size validation passed"
                '''
            }
        }
        
        stage('Store Artifacts') {
            steps {
                script {
                    // Generate build metadata
                    def buildTimestamp = new Date().format('yyyyMMdd-HHmmss')
                    def romName = "encom-64-build-${env.BUILD_NUMBER}-${buildTimestamp}.z64"
                    
                    sh """
                        # Rename ROM with build info
                        cp encom-64.z64 ${romName}
                        
                        # Create build metadata
                        cat > build-info.txt << EOF
Build Number: ${env.BUILD_NUMBER}
Build Timestamp: ${buildTimestamp}
Git Commit: \$(git rev-parse HEAD)
Map Seed: \$(jq -r '.metadata.seed // "unknown"' map_response.json)
ROM Size: \$(stat -c%s ${romName}) bytes
EOF
                        
                        echo "Build metadata:"
                        cat build-info.txt
                    """
                    
                    // Archive the ROM and metadata
                    archiveArtifacts artifacts: "${romName}, build-info.txt, map_response.json", 
                                   allowEmptyArchive: false
                    
                    echo "ROM archived as: ${romName}"
                }
            }
        }
    }
    
    post {
        always {
            // Clean up temporary files but keep toolchain
            sh '''
                rm -f map_response.json
                rm -f encom-64.z64
                rm -f build-info.txt
                rm -f encom-64-build-*.z64
            '''
        }
        
        success {
            echo 'ENCOM-64 ROM build completed successfully!'
        }
        
        failure {
            echo 'ENCOM-64 ROM build failed. Check logs for details.'
        }
    }
}