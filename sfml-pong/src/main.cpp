#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <optional>

int main(){
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    const unsigned WINDOW_W = 800, WINDOW_H = 600;
    sf::RenderWindow window(sf::VideoMode(sf::Vector2u{WINDOW_W, WINDOW_H}), "Pong - SFML");
    window.setFramerateLimit(60);

    sf::RectangleShape leftPaddle(sf::Vector2f{10.f, 100.f});
    leftPaddle.setPosition(sf::Vector2f{50.f, WINDOW_H/2.f - 50.f});
    sf::RectangleShape rightPaddle(sf::Vector2f{10.f, 100.f});
    rightPaddle.setPosition(sf::Vector2f{static_cast<float>(WINDOW_W) - 60.f, WINDOW_H/2.f - 50.f});

    float paddleSpeed = 400.f;

    sf::CircleShape ball(8.f);
    ball.setPosition(sf::Vector2f{WINDOW_W/2.f, WINDOW_H/2.f});
    sf::Vector2f ballVel{-300.f, -150.f};

    int scoreLeft = 0, scoreRight = 0;

    sf::Font font;
    bool fontLoaded = font.openFromFile("assets/arial.ttf"); // new SFML3 API
    std::optional<sf::Text> scoreText;
    if(fontLoaded){
        scoreText.emplace(font, "", 36u);
        scoreText->setFillColor(sf::Color::White);
        scoreText->setPosition(sf::Vector2f{WINDOW_W/2.f - 60.f, 10.f});
    } else {
        std::cerr << "Warning: could not open assets/arial.ttf. Scores will not be shown.\n";
    }

    sf::Clock clock;
    while(window.isOpen()){
        float dt = clock.restart().asSeconds();

        // Event handling
        while (const auto eventOpt = window.pollEvent()){
            const auto& event = *eventOpt;
            if (event.is<sf::Event::Closed>())
                window.close();
        }

        // Input (real-time)
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) leftPaddle.move(sf::Vector2f{0.f, -paddleSpeed * dt});
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) leftPaddle.move(sf::Vector2f{0.f, paddleSpeed * dt});
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)) rightPaddle.move(sf::Vector2f{0.f, -paddleSpeed * dt});
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)) rightPaddle.move(sf::Vector2f{0.f, paddleSpeed * dt});

        // Clamp paddles
        auto clamp = [&](sf::RectangleShape &p){
            if(p.getPosition().y < 0.f) p.setPosition(sf::Vector2f{p.getPosition().x, 0.f});
            if(p.getPosition().y + p.getSize().y > static_cast<float>(WINDOW_H)) p.setPosition(sf::Vector2f{p.getPosition().x, static_cast<float>(WINDOW_H) - p.getSize().y});
        };
        clamp(leftPaddle); clamp(rightPaddle);

        // Ball movement
        ball.move(ballVel * dt);

        // Top/bottom collision
        if(ball.getPosition().y <= 0.f) ballVel.y = std::abs(ballVel.y);
        if(ball.getPosition().y + ball.getRadius()*2 >= WINDOW_H) ballVel.y = -std::abs(ballVel.y);

        // Paddle collision (using findIntersection)
        if(ball.getGlobalBounds().findIntersection(leftPaddle.getGlobalBounds())){
            ballVel.x = std::abs(ballVel.x);
            // add a little spin based on where it hit the paddle
            float offset = (ball.getPosition().y + ball.getRadius()) - (leftPaddle.getPosition().y + leftPaddle.getSize().y/2.f);
            ballVel.y = offset * 5.f;
        }
        if(ball.getGlobalBounds().findIntersection(rightPaddle.getGlobalBounds())){
            ballVel.x = -std::abs(ballVel.x);
            float offset = (ball.getPosition().y + ball.getRadius()) - (rightPaddle.getPosition().y + rightPaddle.getSize().y/2.f);
            ballVel.y = offset * 5.f;
        }

        // Scoring
        if(ball.getPosition().x < 0.f){
            scoreRight++;
            ball.setPosition(sf::Vector2f{WINDOW_W/2.f, WINDOW_H/2.f});
            ballVel = sf::Vector2f{-300.f, static_cast<float>(std::rand()%200 - 100)};
        }
        if(ball.getPosition().x > WINDOW_W){
            scoreLeft++;
            ball.setPosition(sf::Vector2f{WINDOW_W/2.f, WINDOW_H/2.f});
            ballVel = sf::Vector2f{300.f, static_cast<float>(std::rand()%200 - 100)};
        }

        if(fontLoaded){
            (*scoreText).setString(std::to_string(scoreLeft) + "  -  " + std::to_string(scoreRight));
        }

        // Draw
        window.clear(sf::Color(20, 20, 20));
        window.draw(leftPaddle);
        window.draw(rightPaddle);
        window.draw(ball);
        if(fontLoaded) window.draw(*scoreText);
        window.display();
    }

    return 0;
}
