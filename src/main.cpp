#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <iostream>
#include <fstream>
#include <asio.hpp>
#include <asio/ssl.hpp>
#include "bot.hpp"
#include <unordered_set>

const int SEARCH_DEPTH = 3;
const int SQUARE_SIZE = 100;

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

bool isWhite(std::string game, std::string username) {
    std::string targetString = "[White \"" + username + "\"]";
    std::vector<std::string> lines = split(game, '\n');
    for(int i = 0; i < lines.size(); i++) {
        if(lines[i] == targetString) {
            return true;
        }
    }
    return false;
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

enum Classification {
    None,
    Book, // Brown
    Good, // Green
    Miss, // Yellow Question Mark
    Mistake, // Orange Question & Exclamation Mark
    Blunder, // Red Double Question Mark
    Brilliant // Turquoise Double Exclamation Mark
};

struct ClassifiedMove {
    std::string move;
    Classification cl;
};

bool isBookMove(Board board, std::string move, const std::unordered_set<std::string>& book) {
    board.makeMove(uci::parseSan(board, move));
    if(book.find(board.getFen()) != book.end()) {
        return true;
    }else{
        return false;
    }
}

void evaluateAllMoves(const std::vector<std::string>& moves, std::vector<EvaluatedMove>& evaluatedMoves) {
    Board board;
    for(int i = 0; i < moves.size(); i++) {
        EvaluatedMove move;
        move.move = moves[i];
        
        Move boardMove = uci::parseSan(board, move.move);
        board.makeMove(boardMove);
        move.evaluation = search(board, SEARCH_DEPTH, 0, -KING_VALUE, KING_VALUE);

        evaluatedMoves.push_back(move);
    }
}

void loadOpeningBook(std::unordered_set<std::string>& book) {
    std::ifstream is("opening_book.txt");
    if(is.fail()) {
        std::cerr << "Failed to load opening book" << std::endl;
    }
    std::string fen;
    while (std::getline(is, fen)) {
        book.insert(fen);
    }
    is.close();
}

void classifyMoves(const std::vector<EvaluatedMove>& evaluatedMoves, const std::unordered_set<std::string>& book, std::vector<ClassifiedMove>& classifiedMoves, bool white) {
    Board board;
    for(int i = 0; i < evaluatedMoves.size(); i++) {
        ClassifiedMove move;
        move.move = evaluatedMoves[i].move;

        if(i % 2 == white) {
            // Dont classify
            move.cl = None;
        }else{
            // Classify
            if(isBookMove(board, evaluatedMoves[i].move, book)) {
                move.cl = Book;
            }else{
                move.cl = Good;
            }
        }


        classifiedMoves.push_back(move);
        board.makeMove(uci::parseSan(board, evaluatedMoves[i].move));
    }
}

void drawBoard(sf::RenderWindow& window) {
    for(int i = 0; i < 8; i++) {
        for(int j = 0; j < 8; j++) {
            sf::RectangleShape rect;
            rect.setPosition(sf::Vector2f(j * SQUARE_SIZE, i * SQUARE_SIZE));
            rect.setSize(sf::Vector2f(SQUARE_SIZE, SQUARE_SIZE));
            if((i + j) % 2 == 0){
                rect.setFillColor(sf::Color(240, 217, 181));
            } else {
                rect.setFillColor(sf::Color(181, 136, 99));
            }
            window.draw(rect);
        }
    }
}

sf::IntRect getTextureRect(sf::Texture* piecesTexture, int i, int j) {
    int pieceWidth = piecesTexture->getSize().x / 6;
    int pieceHeight = piecesTexture->getSize().y / 2;
    return sf::IntRect(sf::Vector2i(j * pieceWidth, i * pieceHeight), sf::Vector2i(pieceWidth, pieceHeight));
}

void drawPieces(sf::RenderWindow& window, const Board& board, sf::Texture* piecesTexture) {
    sf::RectangleShape rect;
    rect.setTexture(piecesTexture);
    for(int i = 0; i < 8; i++) {
        for(int j = 0; j < 8; j++) {
            Square square((7 - i) * 8 + j);
            Piece p = board.at(square);
            if(p == Piece::NONE){
                continue;
            }
            rect.setPosition(sf::Vector2f(j * SQUARE_SIZE, i * SQUARE_SIZE));
            rect.setSize(sf::Vector2f(SQUARE_SIZE, SQUARE_SIZE));
            int yCoord = 0;
            int xCoord = 0;
            if(p.type() == PieceType::PAWN) {
                xCoord = 5;
            } else if(p.type() == PieceType::BISHOP) {
                xCoord = 2;
            } else if(p.type() == PieceType::KNIGHT) {
                xCoord = 3;
            } else if(p.type() == PieceType::ROOK) {
                xCoord = 4;
            } else if(p.type() == PieceType::QUEEN) {
                xCoord = 1;
            }
            if(p.color() == Color::BLACK) {
                yCoord = 1;
            }
            rect.setTextureRect(getTextureRect(piecesTexture, yCoord, xCoord));
            window.draw(rect);
        }
    }
}

void drawSquare(sf::RenderWindow& window, Square square) {
    sf::RectangleShape rect;
    int x = square.file();
    int y = 7 - square.rank();
    rect.setPosition(sf::Vector2f(x * SQUARE_SIZE, y * SQUARE_SIZE));
    rect.setSize(sf::Vector2f(SQUARE_SIZE, SQUARE_SIZE));
    rect.setFillColor(sf::Color(245, 245, 130, 200));
    window.draw(rect);
}

sf::Texture* loadIcon(std::string iconName) {
    sf::Texture* texture = new sf::Texture();
    if(!texture->loadFromFile(std::string("icons/" + iconName + ".png"))){
        std::cerr << "Failed to load " << iconName << " icon" << std::endl;
        return nullptr;
    }
    return texture;
}

void drawIcon(sf::RenderWindow& window, Square square, Classification cl, sf::Texture* icons[]) {
    sf::RectangleShape rect;
    int x = square.file() + 1;
    int y = 7 - square.rank();
    float xPos = x * SQUARE_SIZE;
    float yPos = y * SQUARE_SIZE;
    rect.setOrigin(sf::Vector2f(16, 16));
    rect.setPosition(sf::Vector2f(xPos, yPos));
    rect.setSize(sf::Vector2f(32, 32));
    rect.setTexture(icons[cl-1]);
    window.draw(rect);
}

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

    bool white = true;

    // Evaluate every move
    std::vector<EvaluatedMove> evaluatedMoves;
    evaluateAllMoves(moves, evaluatedMoves);

    // Load opening book
    std::unordered_set<std::string> book;
    loadOpeningBook(book);

    // Classify every other move based on whether we are playing black or white
    std::vector<ClassifiedMove> classifiedMoves;
    classifyMoves(evaluatedMoves, book, classifiedMoves, white);

    // Create window and start loop
    sf::RenderWindow window(sf::VideoMode({800, 800}), "ChessReview");
    window.setFramerateLimit(60);

    sf::Texture piecesTexture;
    if(!piecesTexture.loadFromFile("pieces.png")){
        std::cerr << "Failed to load pieces texture" << std::endl;
        return -1;
    }
    piecesTexture.setSmooth(true);
    if(!piecesTexture.generateMipmap()){
        std::cout << "Warning: could not generate mipmap texture, render quality may suffer";
    }

    sf::Texture* icons[6];
    icons[0] = loadIcon("book");
    icons[1] = loadIcon("good");
    icons[2] = loadIcon("miss");
    icons[3] = loadIcon("mistake");
    icons[4] = loadIcon("blunder");
    icons[5] = loadIcon("brilliant");

    Board board;
    std::vector<Move> moveHistory;

    while (window.isOpen())
    {
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>()) {
                delete icons[0];
                delete icons[1];
                delete icons[2];
                delete icons[3];
                delete icons[4];
                delete icons[5];
                window.close();
            } else if (event->is<sf::Event::KeyPressed>()) {
                auto keyEvent = event->getIf<sf::Event::KeyPressed>();
                if(keyEvent->code == sf::Keyboard::Key::Right) {
                    if(moveHistory.size() < classifiedMoves.size()){
                        Move move = uci::parseSan(board, classifiedMoves[moveHistory.size()].move);
                        board.makeMove(move);
                        moveHistory.push_back(move);
                    }
                }else if(keyEvent->code == sf::Keyboard::Key::Left) {
                    if(moveHistory.size() > 0) {
                        board.unmakeMove(moveHistory.back());
                        moveHistory.pop_back();
                    }
                }
            }
        }

        window.clear();
        drawBoard(window);

        if(moveHistory.size() > 0) {
            drawSquare(window, moveHistory.back().from());
            drawSquare(window, moveHistory.back().to());
            if(classifiedMoves[moveHistory.size()-1].cl != None) {
                drawIcon(window, moveHistory.back().to(), classifiedMoves[moveHistory.size()-1].cl, icons);
            }
        }

        drawPieces(window, board, &piecesTexture);
        window.display();
    }
}