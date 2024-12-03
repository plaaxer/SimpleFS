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
            "Format", "Mount", "Debug", "Create", "Delete",
            "Cat", "CopyIn", "CopyOut", "Help", "Exit"
        };

        for (size_t i = 0; i < commands.size(); ++i) {

            sf::RectangleShape button(sf::Vector2f(150, 50));
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
        if (command == "Exit") {
            window.close();
        } else if (command == "Format") {
            outputText += "> Executed: " + command + "\n";
            consoleOutput.setString(outputText);
            if (filesystem->fs_format()) {
                outputText += "Disk formatted successfully.\n";
            } else {
                outputText += "Format failed!\n";
            }
        } else if (command == "Mount") {
            outputText += "> Executed: " + command + "\n";
            consoleOutput.setString(outputText);
            if (filesystem->fs_mount()) {
                outputText += "Disk mounted successfully.\n";
            } else {
                outputText += "Mount failed!\n";
            }
        } else if (command == "Debug") {
            outputText += "> Executed: " + command + "\n";
            consoleOutput.setString(outputText);
            filesystem->fs_debug();
        } else if (command.rfind("GetSize ", 0) == 0) {
            int inumber = std::stoi(command.substr(8));
            int result = filesystem->fs_getsize(inumber);
            if (result >= 0) {
                outputText += "Inode " + std::to_string(inumber) + " has size " + std::to_string(result) + ".\n";
            } else {
                outputText += "GetSize failed!\n";
            }
        } else if (command == "Create") {
            int inumber = filesystem->fs_create();
            if (inumber > 0) {
                outputText += "Created inode " + std::to_string(inumber) + ".\n";
            } else {
                outputText += "Create failed!\n";
            }
        } else if (command == "Delete") {
            setupConsole("Enter the inode number to delete: ", [this](const std::string& input) {
            try {
                int inumber = std::stoi(input);
                if (filesystem->fs_delete(inumber)) {
                    outputText += "Inode " + std::to_string(inumber) + " deleted.\n";
                } else {
                    outputText += "Delete failed!\n";
                }
            } catch (std::invalid_argument& e) {
                outputText += "Invalid input. Please enter a valid inode number.\n";
            }
            consoleOutput.setString(outputText);
        });
        } else if (command == "Help") {
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
};
    
} // namespace graphic_interface