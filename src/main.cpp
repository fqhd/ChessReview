#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <iostream>
#include <fstream>
#include <asio.hpp>
#include <asio/ssl.hpp>
#include "bot.hpp"

const int SEARCH_DEPTH = 3;

std::string getUsername() {
    std::string username;
    // Check if the file exists
    std::ifstream is("username.txt");
    if(is.fail()) {
        is.close();
        // Prompt user to enter username and save it inside a file called username.txt
        std::cout << "Enter your Lichess Username: ";
        std::cin >> username;
        std::ofstream os("username.txt");
        os << username;
        os.close();
        return username;
    } else {
        is >> username;
        is.close();
        return username;
    }

}

std::string request(std::string host, std::string port, std::string target) {
    try {
        // Set up I/O context
        asio::io_context io_context;

        // Create SSL context and set it to use the default certificate paths
        asio::ssl::context ssl_context(asio::ssl::context::sslv23_client);

        // Create the SSL socket
        asio::ssl::stream<asio::ip::tcp::socket> ssl_socket(io_context, ssl_context);

        // Resolve the host and port
        asio::ip::tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve(host, port);

        // Connect the socket
        asio::connect(ssl_socket.lowest_layer(), endpoints);

        // Perform SSL handshake
        ssl_socket.handshake(asio::ssl::stream_base::client);

        // Form the HTTP GET request
        std::string http_request =
            "GET " + target + " HTTP/1.1\r\n" +
            "Host: " + host + "\r\n" +
            "User-Agent: AsioClient/1.0\r\n" + // Optional: Add a user-agent header
            "Connection: close\r\n\r\n";       // Ensure the request ends correctly

        // Send the request
        asio::write(ssl_socket, asio::buffer(http_request));

        // Read the response
        asio::streambuf response_buffer;
        asio::error_code ec;
        std::ostringstream response_stream;

        while (asio::read(ssl_socket, response_buffer, asio::transfer_at_least(1), ec)) {
            response_stream << &response_buffer;
        }

        // Check for EOF, which indicates the response was read completely
        if (ec != asio::error::eof && ec) {
            std::cerr << "Error: " << ec.message() << std::endl;
            return "";
        }

        // Return the response as a string
        return response_stream.str();

    } catch (const asio::system_error& e) {
        // Print error message and return empty string
        std::cerr << "Error: " << e.code().message() << std::endl;
        return "";
    }
}

std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> result;
    std::stringstream ss(str);
    std::string item;
    while (std::getline(ss, item, delimiter)) {
        result.push_back(item);
    }
    return result;
}

std::string getPGN(std::string game) {
    std::vector<std::string> lines = split(game, '\n');
    for(int i = 0; i < lines.size(); i++) {
        if (lines[i][0] == '1' && lines[i][1] == '.' && lines[i][2] == ' ') {
            return lines[i];
        }
    }
    return "";
}

std::vector<std::string> parsePGN(const std::string& pgn) {
    std::vector<std::string> moves;
    std::stringstream ss(pgn);
    std::string token;

    while (ss >> token) {
        // Ignore the move number (e.g., "1.", "2.", etc.)
        if (token.back() == '.') {
            continue;
        }
        // Check if the token represents the result (e.g., "0-1", "1-0", "1/2-1/2")
        if (token == "0-1" || token == "1-0" || token == "1/2-1/2") {
            break;
        }
        moves.push_back(token);
    }

    return moves;
}

struct EvaluatedMove {
    std::string move;
    int evaluation;
};

int main()
{

    // Get username
    // std::string username = getUsername();

    // std::string game = request("lichess.org", "443", "/api/games/user/" + username + "?max=1&opening=false");
    // if (game == "") {
    //     std::cerr << "Failed to retrieve game" << std::endl;
    //     return -1;
    // }

    // std::string pgn = getPGN(game);
    // if (pgn == "") {
    //     std::cerr << "Failed to parse game data" << std::endl;
    //     return -1;
    // }

    // std::vector<std::string> moves = parsePGN(pgn);

    std::vector<std::string> moves = {
        "e4",
        "e5",
        "Nf3",
        "Qf6",
        "Bc4",
        "d6",
        "Nc3"
    };

    // Evaluate every move
    Board board;
    std::vector<EvaluatedMove> evaluatedMoves;
    for(int i = 0; i < moves.size(); i++) {
        EvaluatedMove move;
        move.move = moves[i];
        
        Move boardMove = uci::parseSan(board, move.move);
        board.makeMove(boardMove);
        move.evaluation = search(board, SEARCH_DEPTH, 0, -KING_VALUE, KING_VALUE);

        evaluatedMoves.push_back(move);
    }

    for(int i = 0; i < evaluatedMoves.size(); i++) {
        std::cout << evaluatedMoves[i].move << ": " << evaluatedMoves[i].evaluation << std::endl;
    }

    // Classify every other move based on whether we are playing black or white

    // Classify moves(Book, Miss, Inaccuracy, Blunder, Brilliant)

    // Create window and start loop

    sf::RenderWindow window(sf::VideoMode({200, 200}), "SFML works!");
    sf::CircleShape shape(100.f);
    shape.setFillColor(sf::Color::Green);

    while (window.isOpen())
    {
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
                window.close();
        }

        window.clear();
        window.draw(shape);
        window.display();
    }
}