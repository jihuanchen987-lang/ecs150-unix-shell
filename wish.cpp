#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <cerrno>
#include <vector>
#include <string>
#include <fcntl.h>
void print_error() {
    const char message[] = "An error has occurred\n";
    write(STDERR_FILENO, message, sizeof(message) - 1);
}
int main(int argc, char *argv[]) {
if (argc > 2) {
    print_error();
    return 1;
}

FILE *input = stdin;
if (argc == 2) {
    input = fopen(argv[1], "r");
    if (input == nullptr) {
        print_error();
        return 1;
    }
}
    char *line = nullptr;
    size_t capacity = 0;
std::vector<std::string> paths = {"/bin"};
    while (true) {
if (argc == 1) {
    printf("wish> ");
    fflush(stdout);
}

        if (getline(&line, &capacity, input) == -1) {
            break;
        }
std::vector<std::string> commands;
std::string full_line = line;
size_t start = 0;

while (true) {
    size_t end = full_line.find('&', start);
    commands.push_back(full_line.substr(start, end - start));

    if (end == std::string::npos) {
        break;
    }
    start = end + 1;
}
std::vector<pid_t> children;
bool exit_requested = false;
for (std::string &command : commands) {
        std::vector<char *> args;
std::string prepared;
for (const char *p = command.c_str(); *p != '\0'; ++p) {
    if (*p == '>') {
        prepared += " > ";
    } else {
        prepared += *p;
    }
}

char *token = strtok(prepared.data(), " \t\r\n");

while (token != nullptr) {
    args.push_back(token);
    token = strtok(nullptr, " \t\r\n");
}

if (args.empty()) {
    continue;
}

std::string output_file;
bool invalid_redirect = false;

for (size_t i = 0; i < args.size(); ++i) {
    if (strcmp(args[i], ">") == 0) {
        if (i == 0 || i + 2 != args.size() ||
            strcmp(args[i + 1], ">") == 0) {
            invalid_redirect = true;
        } else {
            output_file = args[i + 1];
            args.resize(i);
        }
        break;
    }
}

if (invalid_redirect) {
    print_error();
    continue;
}
if (strcmp(args[0], "exit") == 0) {
    if (args.size() != 1) {
        print_error();
        continue;
    }
    exit_requested = true;
    break;
}
if (strcmp(args[0], "cd") == 0) {
    if (args.size() != 2) {
        print_error();
    } else if (chdir(args[1]) != 0) {
        print_error();
    }
    continue;
}

if (strcmp(args[0], "path") == 0) {
    paths.clear();
    for (size_t i = 1; i < args.size(); ++i) {
        paths.push_back(args[i]);
    }
    continue;
}
args.push_back(nullptr);
    pid_t pid = fork();

if (pid < 0) {
    print_error();
} else if (pid == 0) {
    if (!output_file.empty()) {
    int fd = open(output_file.c_str(),
                  O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) {
        print_error();
        _exit(1);
    }

    if (dup2(fd, STDOUT_FILENO) == -1 ||
        dup2(fd, STDERR_FILENO) == -1) {
        close(fd);
        print_error();
        _exit(1);
    }

    close(fd);
}
for (const std::string &directory : paths) {
    std::string executable = directory + "/" + args[0];

    if (access(executable.c_str(), X_OK) == 0) {
        execv(executable.c_str(), args.data());
        break;
    }
}
    print_error();
    _exit(1);
} else {
   children.push_back(pid);
}  // 结束父进程分支
}  // 结束命令循环

for (pid_t child : children) {
    while (waitpid(child, nullptr, 0) == -1) {
        if (errno != EINTR) {
            print_error();
            break;
        }
    }
}

if (exit_requested) {
    break;
}
}  // 结束输入循环 

    free(line);
if (input != stdin) {
    fclose(input);
}    
    return 0;
}