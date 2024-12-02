#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <functional>

class SimpleFSInterface {
public:
    SimpleFSInterface() {
        // Configurações básicas da janela
        window.create(sf::VideoMode(800, 600), "SimpleFS Interface");
        font.loadFromFile("arial.ttf");

        // Configuração inicial dos botões e textos
        setupButtons();
    }

    void run() {
        while (window.isOpen()) {
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    window.close();
                }

                // Detectar cliques nos botões
                handleButtonClick(event);
            }

            render();
        }
    }

private:
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
            // Configuração dos botões
            sf::RectangleShape button(sf::Vector2f(150, 50));
            button.setPosition(50, 50 + i * 60);
            button.setFillColor(sf::Color::Blue);
            buttons.push_back(button);

            // Configuração dos textos dos botões
            sf::Text text;
            text.setFont(font);
            text.setString(commands[i]);
            text.setCharacterSize(20);
            text.setFillColor(sf::Color::White);
            text.setPosition(button.getPosition().x + 20, button.getPosition().y + 10);
            buttonTexts.push_back(text);
        }

        // Configuração do console de saída
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

    void executeCommand(const std::string& command) {
        if (command == "Exit") {
            window.close();
        } else {
            outputText += "> Executed: " + command + "\n";
            consoleOutput.setString(outputText);
            // Aqui você conectaria os comandos do shell com as funções do sistema de arquivos
            std::cout << "Simulating command: " << command << std::endl;
        }
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

int main() {
    SimpleFSInterface interface;
    interface.run();

    return 0;
}
