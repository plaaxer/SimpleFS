#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <functional>

#include "fs.h"

namespace graphic_interface {

class SimpleFSInterface {
public:
    SimpleFSInterface(INE5412_FS* fs) : filesystem(fs) {

        window.create(sf::VideoMode(1280, 720), "SimpleFS Interface");


        if (!font.loadFromFile("airstrike.ttf")) {
            std::cerr << "Error loading font\n";
        }

        setupButtons();
    }

    void run() {
        while (window.isOpen()) {
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    window.close();
                }
                handleButtonClick(event);
            }

            render();
        }
    }

    void printOutput(const std::string& output) {
        outputText += output + "\n";
        consoleOutput.setString(outputText);
    }

private:
    INE5412_FS* filesystem;
    sf::RenderWindow window;
    sf::Font font;
    std::vector<sf::RectangleShape> buttons;
    std::vector<sf::Text> buttonTexts;
    sf::Text consoleOutput;
    std::string outputText;

    void setupButtons() {
        std::vector<std::string> commands = {
            "Format", "Mount", "Debug", "GetSize", "Create", "Delete",
            "Cat", "CopyIn", "CopyOut", "Help/Clear", "Exit"
        };

        for (size_t i = 0; i < commands.size(); ++i) {

            sf::RectangleShape button(sf::Vector2f(160, 50));
            button.setPosition(50, 50 + i * 60);
            button.setFillColor(sf::Color::Blue);
            buttons.push_back(button);

            sf::Text text;
            text.setFont(font);
            text.setString(commands[i]);
            text.setCharacterSize(20);
            text.setFillColor(sf::Color::White);
            text.setPosition(button.getPosition().x + 20, button.getPosition().y + 10);
            buttonTexts.push_back(text);
        }

        consoleOutput.setFont(font);
        consoleOutput.setCharacterSize(16);
        consoleOutput.setFillColor(sf::Color::White);
        consoleOutput.setPosition(250, 50);
        outputText = "Output Console:\n";
        consoleOutput.setString(outputText);
    }
    

    void handleButtonClick(sf::Event& event) {
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            sf::Vector2i mousePos = sf::Mouse::getPosition(window);

            for (size_t i = 0; i < buttons.size(); ++i) {
                if (buttons[i].getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePos))) {
                    executeCommand(buttonTexts[i].getString());
                    break;
                }
            }
        }
    }
    void setupConsole(const std::string& prompt, std::function<void(const std::string&)> callback) {
        sf::RenderWindow inputWindow(sf::VideoMode(400, 200), "Input Prompt");
        sf::Font localFont;

        if (!localFont.loadFromFile("airstrike.ttf")) {
            std::cerr << "Error loading font for console input.\n";
        }

        sf::Text promptText;
        promptText.setFont(localFont);
        promptText.setString(prompt);
        promptText.setCharacterSize(16);
        promptText.setFillColor(sf::Color::White);
        promptText.setPosition(20, 50);

        std::string userInput;
        sf::Text inputText;
        inputText.setFont(localFont);
        inputText.setCharacterSize(16);
        inputText.setFillColor(sf::Color::White);
        inputText.setPosition(20, 100);

        while (inputWindow.isOpen()) {
            sf::Event event;
            while (inputWindow.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    inputWindow.close();
                    return; // Cancel the input
                }

                if (event.type == sf::Event::TextEntered) {
                    if (event.text.unicode == '\b') { // Handle backspace
                        if (!userInput.empty())
                            userInput.pop_back();
                    } else if (event.text.unicode == '\r') { // Handle Enter key
                        inputWindow.close();
                        callback(userInput); // Pass input to callback
                        return;
                    } else if (event.text.unicode < 128) {
                        userInput += static_cast<char>(event.text.unicode);
                    }
                    inputText.setString(userInput);
                }
            }

            inputWindow.clear(sf::Color::Black);
            inputWindow.draw(promptText);
            inputWindow.draw(inputText);
            inputWindow.display();
        }
}

    void executeCommand(const std::string& command) {
        outputText += "> Executed: " + command + "\n";
        if (command == "Exit") {
            window.close();
        } else if (command == "Format") {
            int result = filesystem->fs_format();
            if (result) {
                outputText += "Disk formatted successfully.\n";
            } else {
                outputText += "Error: cannot format a mounted disk!\n";
            }
        } else if (command == "Mount") {
            int result = filesystem->fs_mount();
            if (result == 1) {
                outputText += "Disk mounted successfully.\n";
            } else if (result == 0) {
                outputText += "Error: disk already mounted!\n";
            } else if (result == -1) {
                outputText += "Error: the disk's magic number was not found!\n";
            } else {
                outputText += "Mount failed!\n";
            }
        } else if (command == "Debug") {
            outputText += filesystem->fs_debug();
            
        } else if (command == "GetSize") {
            setupConsole("Enter the inode number to get size: ", [this](const std::string& input) {
                try {
                    int inumber;
                    try {
                    inumber = std::stoi(input);
                    } catch (std::out_of_range& e) {
                        outputText += "Invalid input. Please enter a valid inode number.\n";
                        consoleOutput.setString(outputText);
                        return;
                    }
                    int size = filesystem->fs_getsize(inumber);
                    if (size == -1) {
                        outputText += "Error: disk not mounted!\n";
                    } else if (size == -2) {
                        outputText += "Error: invalid inode number!\n";
                    } else {
                        outputText += "Size of inode " + std::to_string(inumber) + ": " + std::to_string(size) + "\n";
                    }
                } catch (std::invalid_argument& e) {
                    outputText += "Invalid input. Please enter a valid inode number.\n";
                }
                consoleOutput.setString(outputText);
            });
        } else if (command == "Create") {
            int inumber = filesystem->fs_create();
            if (inumber > 0) {
                outputText += "Created inode " + std::to_string(inumber) + ".\n";
            } else if (inumber == 0) {
                outputText += "Error: disk not mounted!\n";
            } else if (inumber == -1) {
                outputText += "Error: no free inodes available!\n";
            } else {
                outputText += "Create failed!\n";
            }
        } else if (command == "Delete") {
            setupConsole("Enter the inode number to delete: ", [this](const std::string& input) {
            try {
                int inumber;
                try {
                    inumber = std::stoi(input);
                } catch (std::out_of_range& e) {
                    outputText += "Invalid input. Please enter a valid inode number.\n";
                    consoleOutput.setString(outputText);
                    return;
                    }
                int result = filesystem->fs_delete(inumber);
                if (result == 1) {
                    outputText += "Inode " + std::to_string(inumber) + " deleted.\n";
                } else if (result == 0) {
                    outputText += "Error: disk not mounted!\n";
                } else if (result == -1) {
                    outputText += "Error: invalid inode number!\n";
                } else {
                    outputText += "Delete failed!\n";
                }
            } catch (std::invalid_argument& e) {
                outputText += "Invalid input. Please enter a valid inode number.\n";
            }
            consoleOutput.setString(outputText);
        });

        } else if (command == "Cat") {
            cout << "Cat command\n";
            setupConsole("Enter the inode number to read: ", [this](const std::string& input) {
                int inumber;
                try {
                    inumber = std::stoi(input);
                } catch (std::invalid_argument&) {
                    outputText += "Invalid input. Please enter a valid inode number.\n";
                    consoleOutput.setString(outputText);
                    return;
                } catch (std::out_of_range& e) {
                    outputText += "Invalid input. Please enter a valid inode number.\n";
                    consoleOutput.setString(outputText);
                    return;
                }
                int result = do_copyout(inumber, "/dev/stdout", filesystem);
                if (result == 1) {
                    outputText += "Cat done successfully!\n";               
                } else if (result == 0) {
                    outputText += "Error to open stdout\n";
                } else if (result == -1) {
                    outputText += "Error: disk not mounted!\n";
                } else if (result == -2) {
                    outputText += "Error: inumber not valid!\n";
                } else if (result == -3) {
                    outputText += "Error: inode is not valid!\n";
                } else {
                    outputText += "CopyIn failed!\n";
                }

            });
        } else if (command == "CopyIn") {
            setupConsole("Enter the inode number and file to read: ", [this](const std::string& input) {
                std::istringstream iss(input);
                int inumber;
                std::string path;
                if (!(iss >> inumber >> path)) {
                    outputText += "Invalid input. Please enter a valid inode number and file path.\n";
                    consoleOutput.setString(outputText);
                    return;
                }
                int result = do_copyin(path.c_str(), inumber, filesystem);
                if(result == 1) {
                    outputText += "Copied file " + path + " to inode " + std::to_string(inumber) + "\n";
                } else if (result == 0) {
                    outputText += "Error to open " + path + "\n";
                } else if (result == -1) {
                    outputText += "Error: disk not mounted!\n";
                } else if (result == -2) {
                    outputText += "Error: inumber not valid!\n";
                } else if (result == -3) {
                    outputText += "Error: inode is not valid!\n";
                } else {
                    outputText += "CopyIn failed!\n";
                }
            });
        } else if (command == "CopyOut") {
            cout << "copyout command\n";
            setupConsole("Enter the inode number and file to write: ", [this](const std::string& input) {
                std::istringstream iss(input);
                int inumber;
                std::string path;
                if (!(iss >> inumber >> path)) {
                    outputText += "Invalid input. Please enter a valid inode number and file path.\n";
                    consoleOutput.setString(outputText);
                    return;
                }
                int result = do_copyout(inumber, path.c_str(), filesystem);
                if (result == 1) {
                    outputText += "CopyOut done successfully!\n";               
                } else if (result == 0) {
                    outputText += "Error to open " + path + "\n";
                } else if (result == -1) {
                    outputText += "Error: disk not mounted!\n";
                } else if (result == -2) {
                    outputText += "Error: inumber not valid!\n";
                } else if (result == -3) {
                    outputText += "Error: inode is not valid!\n";
                } else {
                    outputText += "CopyIn failed!\n";
                }
            });
        } else if (command == "Help/Clear") {
            outputText = "Output Console:\n"; // resetando só
            outputText += "Commands are:\n";
            outputText += "    Format\n";
            outputText += "    Mount\n";
            outputText += "    Debug\n";
            outputText += "    Create\n";
            outputText += "    Delete <inode>\n";
            outputText += "    Cat <inode>\n";
            outputText += "    CopyIn <file> <inode>\n";
            outputText += "    CopyOut <inode> <file>\n";
            outputText += "    Help\n";
            outputText += "    Exit\n";
        } else {
            outputText += "Unknown command: " + command + "\n";
            outputText += "Type 'Help' for a list of commands.\n";
        }

        consoleOutput.setString(outputText);
    }


    void render() {
        window.clear(sf::Color::Black);

        for (const auto& button : buttons) {
            window.draw(button);
        }

        for (const auto& text : buttonTexts) {
            window.draw(text);
        }

        window.draw(consoleOutput);
        window.display();
    }

int do_copyin(const char *filename, int inumber, INE5412_FS *fs)
{
	FILE *file;
	int offset=0, result, actual;
	char buffer[16384];

	file = fopen(filename, "r");
	if(!file) {
		cout << "couldn't open " << filename << "\n";
		return 0;
	}

	while(1) {
		result = fread(buffer,1,sizeof(buffer),file);
		if(result <= 0) break;
		if(result > 0) {
			actual = fs->fs_write(inumber,buffer,result,offset);
			if(actual<0) {
				cout << "ERROR: fs_write return invalid result " << actual << "\n";
				break;
			}
			offset += actual;
			if(actual!=result) {
				cout << "WARNING: fs_write only wrote " << actual << " bytes, not " << result << " bytes\n";
				break;
			}
		}
	}

	cout << offset << " bytes copied\n";

    fclose(file);

    if (result < 0) {
        return result;
    }

	return 1;
}



    int do_copyout(int inumber, const char *filename, INE5412_FS *fs)
{
        FILE *file;
        int offset = 0, result;
        char buffer[16384];

        file = fopen(filename,"w");
        if(!file) {
            cout << "couldn't open " << filename << "\n";
            return 0;
        }

        while(1) {
            result = fs->fs_read(inumber,buffer,sizeof(buffer),offset);
            if(result<=0) break;
            fwrite(buffer,1,result,file);
            offset += result;
        }

        cout << offset << " bytes copied\n";

        fclose(file);
        if (result < 0) {
            return result;
        }

        return 1;
}
};
    
} // namespace graphic_interface