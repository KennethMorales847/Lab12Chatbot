#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <map>
#include <vector>
#include <algorithm>
#include <cctype>
#include <unordered_set>

using namespace std;

// Función para eliminar acentos y caracteres especiales
char foldAccent(char c) {
    switch (static_cast<unsigned char>(c)) {
    case '\xC1': case '\xE1': return 'a';  // Á, á
    case '\xC9': case '\xE9': return 'e';  // É, é
    case '\xCD': case '\xED': return 'i';  // Í, í
    case '\xD3': case '\xF3': return 'o';  // Ó, ó
    case '\xDA': case '\xFA': return 'u';  // Ú, ú
    case '\xD1': case '\xF1': return 'n';  // Ñ, ñ
    default: return c;
    }
}

// Normaliza cadena: minúsculas, acentos y signos de puntuación
string normalize(const string& s) {
    string res;
    for (unsigned char c : s) {
        if (c == '¿' || c == '¡') {
            continue;
        }
        else if (ispunct(c)) {
            res += ' ';
        }
        else {
            char lo = tolower(c);
            res += foldAccent(lo);
        }
    }
    return res;
}

// Lista de stopwords en español (puedes ajustar quitando más palabras)
static const unordered_set<string> STOPWORDS = {
    "el","la","los","las","un","una","unos","unas",
    "y","o","de","del","en","para","por","con"
};

// Tokeniza cadena normalizada y elimina stopwords
vector<string> tokenize(const string& s) {
    vector<string> tokens;
    string norm = normalize(s);
    istringstream iss(norm);
    string word;
    while (iss >> word) {
        if (STOPWORDS.find(word) == STOPWORDS.end()) {
            tokens.push_back(word);
        }
    }
    return tokens;
}

// Carga conocimiento en un mapa pregunta->respuesta
void cargarConocimiento(map<string, string>& conocimiento,
    const string& nombreArchivo) {
    ifstream archivo(nombreArchivo);
    if (!archivo.is_open()) {
        cerr << "Error al abrir el archivo de conocimiento: "
            << nombreArchivo << endl;
        return;
    }
    string linea;
    while (getline(archivo, linea)) {
        size_t sep = linea.find('|');
        if (sep != string::npos) {
            string pregunta = linea.substr(0, sep);
            string respuesta = linea.substr(sep + 1);
            conocimiento[pregunta] = respuesta;
        }
    }
    archivo.close();
}

// Búsqueda exacta
string buscarExacto(const map<string, string>& conocimiento,
    const string& pregunta) {
    auto it = conocimiento.find(pregunta);
    if (it != conocimiento.end()) {
        return it->second;
    }
    return "";
}

// Búsqueda por palabras clave con umbral dinámico
string buscarPorPalabrasClave(const map<string, string>& conocimiento,
    const string& pregunta) {
    auto tokensPregunta = tokenize(pregunta);
    int bestScore = 0;
    string bestAnswer;

    for (const auto& par : conocimiento) {
        auto tokensConoc = tokenize(par.first);
        int score = 0;
        for (const auto& tp : tokensPregunta) {
            score += count(tokensConoc.begin(), tokensConoc.end(), tp);
        }
        if (score > bestScore) {
            bestScore = score;
            bestAnswer = par.second;
        }
    }

    // Umbral dinámico: preguntas cortas requieren menos coincidencias
    int minScore = (static_cast<int>(tokenize(pregunta).size()) < 3) ? 1 : 2;
    if (bestScore >= minScore) {
        return bestAnswer;
    }
    return "";
}

int main() {
    map<string, string> conocimiento;
    cargarConocimiento(conocimiento, "conocimiento.txt");

    // Conjunto de saludos normalizados
    static const unordered_set<string> GREETINGS = {
        "hola", "buenas", "buenas tardes", "como estas", "como estas"
    };

    cout << "Bienvenido al Chatbot mejorado." << endl;
    cout << "Escribe 'salir' para terminar." << endl;

    string preguntaUsuario;
    while (true) {
        cout << "\nTu: ";
        getline(cin, preguntaUsuario);
        if (preguntaUsuario == "salir") {
            cout << "Bot: ¡Hasta luego!" << endl;
            break;
        }

        string norm = normalize(preguntaUsuario);
        // Eliminar espacios extras
        istringstream iss(norm);
        string temp, joined;
        while (iss >> temp) {
            if (!joined.empty()) joined += ' ';
            joined += temp;
        }
        // Respuesta de saludo prioritaria
        if (GREETINGS.count(joined)) {
            cout << "Bot: ¡Estoy muy bien, gracias! ¿Y tú?" << endl;
            continue;
        }

        // Búsqueda exacta
        string respuesta = buscarExacto(conocimiento, preguntaUsuario);
        // Búsqueda por palabras clave
        if (respuesta.empty()) {
            respuesta = buscarPorPalabrasClave(conocimiento, preguntaUsuario);
        }
        // Respuesta por defecto
        //Conectar a chatgpt
        if (respuesta.empty()) {
            respuesta = "Lo siento, no conozco la respuesta a esa pregunta.";
        }
        cout << "Bot: " << respuesta << endl;
    }
    return 0;
}
