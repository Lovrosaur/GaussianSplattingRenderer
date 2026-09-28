#include <shader.h>

void Shader::compile() {
    char *source;
    int success;
    std::string vertName(name), fragName(name), geomName(name);
    char infoLog[512];
    vertName.append(".vert");
    fragName.append(".frag");
    geomName.append(".geom");
    vertName = "shaders/" + vertName;
    fragName = "shaders/" + fragName;
    geomName = "shaders/" + geomName;
    GLuint vertShader = -1, geomShader = -1, fragShader = -1; 
    int size = 0;
    if (FILE* file = fopen(vertName.c_str(), "r")) {
        fseek(file, 0L, SEEK_END);
        size = ftell(file);
        fseek(file, 0L, SEEK_SET);	
        source = new char[size+1];
        size = fread(source, sizeof(char), size, file);
        source[size] = '\0';
        fclose(file);

        vertShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertShader,1,&source,&size);
        glCompileShader(vertShader);

        delete source;

        glGetShaderiv(vertShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(vertShader, 512, NULL, infoLog);
            fprintf(stderr, "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n%s\n",infoLog);
            status = -1;
            return;
        }
    } else {
        status = -1;
        return;
    }
    
    if (FILE* file = fopen(fragName.c_str(), "r")) {
        fseek(file, 0L, SEEK_END);
        size = ftell(file);
        fseek(file, 0L, SEEK_SET);	
        source = new char[size+1];
        size = fread(source, sizeof(char), size, file);
        source[size] = '\0';
        fclose(file);

        fragShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragShader,1,&source,NULL);
        glCompileShader(fragShader);
        
        delete source;

        glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(fragShader, 512, NULL, infoLog);
            fprintf(stderr, "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n%s\n",infoLog);
            status = -2;
            return;
        }
    } else {
        status = -2;
        return;
    }

    if (FILE* file = fopen(geomName.c_str(), "r")) {
        fseek(file, 0L, SEEK_END);
        size = ftell(file);
        fseek(file, 0L, SEEK_SET);	
        source = new char[size+1];
        fread(source, sizeof(char), size, file);
        source[size] = '\0';
        fclose(file);

        geomShader = glCreateShader(GL_GEOMETRY_SHADER);
        glShaderSource(geomShader,1,&source,NULL);
        glCompileShader(geomShader);
        
        delete source;

        glGetShaderiv(geomShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(geomShader, 512, NULL, infoLog);
            fprintf(stderr, "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n%s\n",infoLog);
            status = -3;
            return;
        }
    }
    ID = glCreateProgram();
    glAttachShader(ID, vertShader);
    glAttachShader(ID, fragShader);
    if ((int)geomShader != -1) {
        glAttachShader(ID, geomShader);
    }
    glLinkProgram(ID);

    glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(ID, 512, NULL, infoLog);
        fprintf(stderr, "ERROR::SHADER::PROGRAM::LINKING_FAILED\n%s\n",infoLog);
    }
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);
    if ((int)geomShader != -1) {
        glDeleteShader(geomShader);
    }
}

void ComputeShader::compile() {
    char *source;
    int success;
    std::string compName(name);
    char infoLog[512];
    compName.append(".comp");
    compName = "shaders/" + compName;
    GLuint compShader = -1;
    int size = 0;
    if (FILE* file = fopen(compName.c_str(), "r")) {
        fseek(file, 0L, SEEK_END);
        size = ftell(file);
        fseek(file, 0L, SEEK_SET);	
        source = new char[size+1];
        size = fread(source, sizeof(char), size, file);
        source[size] = '\0';
        fclose(file);

        compShader = glCreateShader(GL_COMPUTE_SHADER);
        glShaderSource(compShader,1,&source,&size);
        glCompileShader(compShader);

        delete source;

        glGetShaderiv(compShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(compShader, 512, NULL, infoLog);
            fprintf(stderr, "ERROR::SHADER::COMPUTE::COMPILATION_FAILED\n%s\n",infoLog);
            status = -1;
            return;
        }
    } else {
        status = -1;
        return;
    }
    
    ID = glCreateProgram();
    glAttachShader(ID, compShader);
    glLinkProgram(ID);

    glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(ID, 512, NULL, infoLog);
        fprintf(stderr, "ERROR::SHADER::PROGRAM::LINKING_FAILED\n%s\n",infoLog);
    }
    glDeleteShader(compShader);
}