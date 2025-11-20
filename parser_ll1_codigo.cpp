#include <iostream>
#include <fstream>
#include <utility>
#include <vector>
#include <set>
#include <map>
#include <stack>
#include <algorithm>

using Simbolo = char;
using NaoTerminal = char;

using TabelaAnalise = std::vector<std::vector<std::set<size_t>>>;
using ConjuntoSimbolos = std::map<NaoTerminal, std::set<Simbolo>>;
using Regra = std::pair<NaoTerminal, std::string>;

constexpr Simbolo VAZIO = 'e';
constexpr Simbolo FIM_ENTRADA = '$';

const std::set<char> DELIMITADORES = {
'(',')',
'[',']',
'{','}',
'<','>' };

enum ResultadoValidacao {
    ENTRADA_INVALIDA = -1,
    ACEITA = 1,
    REJEITADA = 0
};

struct Gramatica {
    // a ordem de declaração é importante, pois as funções dependem dos campos anteriores!
    std::vector<Regra> regras;
    std::set<NaoTerminal> nao_terminais;
    std::set<Simbolo> terminais;
    ConjuntoSimbolos first;
    ConjuntoSimbolos follow;
    TabelaAnalise tabela_parsing;

    explicit Gramatica(const std::vector<Regra> &regras_entrada)
        : regras(regras_entrada)
        , nao_terminais{obter_nao_terminais()}
        , terminais{obter_terminais()}
        , first{calcular_first()}
        , follow{calcular_follow()}
        , tabela_parsing{construir_tabela_parsing()}
        {}

    [[nodiscard]] NaoTerminal simbolo_inicial() const {
        return regras.front().first;
    }

    auto operator[](size_t indice) const {
        return regras[indice];
    }

    static bool eh_terminal(char caractere) {
        return !isupper(caractere);
    }

    static bool eh_delimitador(char caractere) {
        return DELIMITADORES.find(caractere) != DELIMITADORES.end();
    }

private:
    void buscar_follow(ConjuntoSimbolos &os_follows, NaoTerminal simbolo) const {
        for (const auto &[lado_esq, lado_dir]: regras) {
            // pula caracteres até encontrar o não-terminal
            auto pos = std::find(lado_dir.begin(), lado_dir.end(), simbolo);
            if (pos == lado_dir.end()) { continue; }
            
            bool finalizado = false;
            for (++pos; pos != lado_dir.end(); ++pos) {
                if (eh_delimitador(*pos)) {
                    continue;
                }
                // se for terminal, apenas adiciona ao FOLLOW
                if (eh_terminal(*pos)) {
                    os_follows[simbolo].insert(*pos);
                    finalizado = true;
                    break;
                }

                const auto& first_pos = first.at(*pos);
                // se os FIRSTs do caractere não têm ε, a busca termina
                if (first_pos.find(VAZIO) == first_pos.end()) {
                    os_follows[simbolo].insert(first_pos.begin(), first_pos.end());
                    finalizado = true;
                    break;
                }
                // caso contrário, próximo caractere deve ser verificado após adicionar FIRSTs
                auto copia_first = first_pos;
                copia_first.erase(VAZIO);
                os_follows[simbolo].insert(copia_first.begin(), copia_first.end());
            }
            if (finalizado) { continue; }
            
            // se chegou ao fim da produção, FOLLOW ⊃ FOLLOW do lado esquerdo
            if (pos == lado_dir.end()) {
                // encontra FOLLOW se não tiver
                if (os_follows[lado_esq].empty()) {
                    buscar_follow(os_follows, lado_esq);
                }
                os_follows[simbolo].insert(os_follows[lado_esq].begin(), os_follows[lado_esq].end());
            }
        }
    }

    void buscar_first(ConjuntoSimbolos &os_firsts, NaoTerminal simbolo) const {
        for (const auto &[lado_esq, lado_dir]: regras) {
            // encontra produções do não-terminal
            if (lado_esq != simbolo) {
                continue;
            }
            
            // loop até encontrar não-terminal ou nenhum ε
            for (auto pos = lado_dir.begin(); pos != lado_dir.end(); ++pos) {
                if (eh_delimitador(*pos)) {
                    continue;
                }
                // se primeiro caractere na produção é terminal, adiciona à lista de firsts
                if (eh_terminal(*pos)) {
                    os_firsts[simbolo].insert(*pos);
                    break;
                }
                
                // se caractere no lado direito é não-terminal e cujo FIRST ainda não foi encontrado
                const auto& first_pos = os_firsts[*pos];
                if (first_pos.empty()) {
                    buscar_first(os_firsts, *pos);
                }
                
                // se variável não tem ε, vai para próxima produção
                if (first_pos.find(VAZIO) == first_pos.end()) {
                    os_firsts[simbolo].insert(first_pos.begin(), first_pos.end());
                    break;
                }
                
                auto copia_first = first_pos;
                // remove ε do FIRST se não for a última variável
                if (!eh_ultimo(pos, lado_dir.end())) {
                    copia_first.erase(VAZIO);
                }
                // anexa firsts dessa variável
                os_firsts[simbolo].insert(copia_first.begin(), copia_first.end());
            }
        }
    }

    template<typename Iterador>
    static bool eh_ultimo(Iterador it, Iterador fim) {
        return std::find_if_not(it, fim, eh_delimitador) != fim;
    }

    [[nodiscard]] ConjuntoSimbolos calcular_first() const {
        ConjuntoSimbolos resultado;
        for (NaoTerminal nt: nao_terminais) {
            if (resultado[nt].empty()) {
                buscar_first(resultado, nt);
            }
        }
        return resultado;
    }

    [[nodiscard]] ConjuntoSimbolos calcular_follow() const {
        ConjuntoSimbolos resultado;
        // encontra follow da variável inicial primeiro
        auto var_inicial = simbolo_inicial();
        resultado[var_inicial].insert(FIM_ENTRADA);
        buscar_follow(resultado, var_inicial);
        
        // encontra follows para resto das variáveis
        for (NaoTerminal nt: nao_terminais) {
            if (resultado[nt].empty()) {
                buscar_follow(resultado, nt);
            }
        }
        return resultado;
    }

    [[nodiscard]] std::set<NaoTerminal> obter_nao_terminais() const {
        std::set<NaoTerminal> resultado;
        for (auto &[lado_esq, _]: regras) {
            resultado.insert(lado_esq);
        }
        return resultado;
    }

    [[nodiscard]] std::set<Simbolo> obter_terminais() const {
        std::set<Simbolo> resultado;
        for (const auto &[_, lado_dir]: regras) {
            for (char c: lado_dir) {
                if (eh_terminal(c) && !eh_delimitador(c)) {
                    resultado.insert(c);
                }
            }
        }
        // remove ε e adiciona caractere de fim $
        resultado.erase(VAZIO);
        resultado.insert(FIM_ENTRADA);
        return resultado;
    }

    [[nodiscard]] TabelaAnalise construir_tabela_parsing() const {
        TabelaAnalise resultado{nao_terminais.size(), std::vector<std::set<size_t>>(terminais.size())};

        size_t num_regra = 0;
        for (const auto &[lado_esq, lado_dir]: regras) {

            std::set<char> lista_proximos;
            bool finalizado = false;
            for (char c: lado_dir) {
                if (eh_delimitador(c)) { continue; }
                if (eh_terminal(c)) {
                    if (c != VAZIO) {
                        lista_proximos.insert(c);
                        finalizado = true;
                        break;
                    }
                    continue;
                }

                auto copia_first = first.at(c);
                if (copia_first.find(VAZIO) == copia_first.end()) {
                    lista_proximos.insert(copia_first.begin(), copia_first.end());
                    finalizado = true;
                    break;
                }
                copia_first.erase(VAZIO);
                lista_proximos.insert(copia_first.begin(), copia_first.end());
            }
            
            // se todo o lado direito pode ser pulado através de ε ou chegando ao fim
            // adiciona FOLLOW à lista PRÓXIMOS
            if (!finalizado) {
                const auto &meus_follows = follow.at(lado_esq);
                lista_proximos.insert(meus_follows.begin(), meus_follows.end());
            }
            
            size_t linha = distance(nao_terminais.begin(), nao_terminais.find(lado_esq));
            for (char c: lista_proximos) {
                size_t coluna = distance(terminais.begin(), terminais.find(c));
                resultado[linha][coluna].insert(num_regra);
            }
            num_regra++;
        }
        return resultado;
    }
};


std::vector<Regra> ler_arquivo(std::istream &arquivo_gramatica) {
    std::vector<Regra> gramatica;

    while (!arquivo_gramatica.eof()) {
        char buffer[20];
        arquivo_gramatica.getline(buffer, sizeof(buffer));
        gramatica.emplace_back(buffer[0], buffer + 3);
    }
    return gramatica;
}

template<typename T>
std::ostream &operator<<(std::ostream &os, const std::set<T> &conjunto) {
    if (conjunto.empty()) {
        return os << "∅";
    }
    os << "{";
    bool primeiro = true;
    for (T elemento: conjunto) {
        if (primeiro) {
            primeiro = false;
        } else {
            os << ", ";
        }
        if (elemento == VAZIO) {
            os << "ε";
        } else {
            os << elemento;
        }
    }
    return os << "}";
}


std::ostream &operator<<(std::ostream &saida, const Gramatica &gram) {
    for (int contador = 0; const auto &[lado_esq, lado_dir]: gram.regras) {
        saida << contador++ << ". " << lado_esq << " -> " 
              << (lado_dir[1] == VAZIO ? (std::string{ lado_dir[0] } + "ε" + lado_dir[2]) : lado_dir) << "\n";
    }
    saida << "\n"
        << "Os não-terminais na gramática são: " << gram.nao_terminais << "\n"
        << "Os terminais na gramática são: " << gram.terminais << "\n"
        << "\n"
        << "Conjunto FIRST: \n";
    for (const auto &[nao_terminal, conjunto]: gram.first) {
        saida << "FIRST(" << nao_terminal << ") = " << conjunto << "\n";
    }
    saida << "\n"
        << "Conjunto FOLLOW: \n";
    for (const auto &[nao_terminal, conjunto]: gram.follow) {
        saida << "FOLLOW(" << nao_terminal << ") = " << conjunto << "\n";
    }
    saida << "\n"
        << "Tabela de Análise Sintática: \n"
        << "\t";
    for (char terminal: gram.terminais) {
        saida << terminal << "\t";
    }
    saida << "\n";
    for (size_t num_linha = 0; NaoTerminal nt: gram.nao_terminais) {
        saida << nt << "\t";
        for (const auto &elemento: gram.tabela_parsing[num_linha++]) {
            saida << elemento << "\t";
        }
        saida << "\n";
    }
    return saida;
}

struct Validador {
    const Gramatica &gram;
    std::vector<std::string> derivacao;

    explicit Validador(const Gramatica &gram) : gram(gram) {
    }

    std::string pilha_para_string(std::stack<char> pilha) {
        std::string resultado;
        std::vector<char> temp;
        
        while (!pilha.empty()) {
            if (pilha.top() != FIM_ENTRADA) {
                temp.push_back(pilha.top());
            }
            pilha.pop();
        }
        
        for (auto it = temp.rbegin(); it != temp.rend(); ++it) {
            if (!Gramatica::eh_delimitador(*it)) {
                resultado += *it;
            }
        }
        
        return resultado.empty() ? "ε" : resultado;
    }

    bool processar(std::string entrada, std::stack<char> pilha) {
        // adiciona forma sentencial inicial
        if (derivacao.empty()) {
            derivacao.push_back(pilha_para_string(pilha));
        }
        
        while (!pilha.empty() && !entrada.empty()) {
            if (Gramatica::eh_delimitador(pilha.top())) {
                pilha.pop();
                continue;
            }

            // se topo da pilha é igual ao caractere da string de entrada, remove
            if (entrada[0] == pilha.top()) {
                pilha.pop();
                entrada.erase(0, 1);
                continue;
            }
            
            if (Gramatica::eh_terminal(pilha.top())) {
                return false;
            }
            
            NaoTerminal topo_pilha = pilha.top();
            pilha.pop();
            
            size_t linha = distance(gram.nao_terminais.begin(), gram.nao_terminais.find(topo_pilha));
            size_t coluna = distance(gram.terminais.begin(), gram.terminais.find(entrada[0]));
            auto nums_regras = gram.tabela_parsing[linha][coluna];

            if (nums_regras.empty()) {
                return false;
            }
            
            if (nums_regras.size() == 1) {
                auto [_, lado_dir] = gram[*nums_regras.begin()];
                if (lado_dir[1] != VAZIO) {
                    for (auto c = lado_dir.rbegin(); c != lado_dir.rend(); ++c) {
                        pilha.push(*c);
                    }
                } else {
                    pilha.push(lado_dir[2]);
                    pilha.push(lado_dir[0]);
                }
                
                // adiciona próxima forma sentencial
                derivacao.push_back(pilha_para_string(pilha));
                continue;
            }

            // se há múltiplas produções, tenta cada uma
            for (size_t numero: nums_regras) {
                auto [_, lado_dir] = gram[numero];
                auto pilha_temp = pilha;
                auto derivacao_temp = derivacao;
                
                if (lado_dir[1] != VAZIO) {
                    for (auto c = lado_dir.rbegin(); c != lado_dir.rend(); ++c) {
                        pilha_temp.push(*c);
                    }
                } else {
                    pilha_temp.push(lado_dir[2]);
                    pilha_temp.push(lado_dir[0]);
                }
                
                derivacao.push_back(pilha_para_string(pilha_temp));
                
                if (processar(entrada, std::move(pilha_temp))) {
                    return true;
                }
                
                derivacao = derivacao_temp; // restaura derivação se falhou
            }
            return false;
        }
        return pilha.empty() || (pilha.size() == 1 && pilha.top() == FIM_ENTRADA);
    }

    static std::pair<ResultadoValidacao, std::vector<std::string>> validar_entrada(std::string entrada,
                                     const Gramatica &gram) {
        entrada.push_back(FIM_ENTRADA);
        std::stack<char> pilha;
        pilha.push(FIM_ENTRADA);
        pilha.push(gram.simbolo_inicial());

        // verifica se a string de entrada é válida
        for (char c: entrada) {
            if (gram.terminais.find(c) == gram.terminais.end()) {
                return {ENTRADA_INVALIDA, {}};
            }
        }

        Validador validador{gram};
        bool aceita = validador.processar(entrada, pilha);
        
        if (aceita) {
            return {ACEITA, validador.derivacao};
        } else {
            return {REJEITADA, {}};
        }
    }
};

void mostrar_resultado(const std::string &texto, const std::pair<ResultadoValidacao, std::vector<std::string>>& resultado) {
    std::cout << '[' << texto << "] ";
    switch (resultado.first) {
        case ENTRADA_INVALIDA:
            std::cout << "contém símbolos desconhecidos";
            break;
        case ACEITA:
            std::cout << "aceita. Derivação: ";
            // Imprime todas as formas sentenciais da derivação
            for (size_t i = 0; i < resultado.second.size(); ++i) {
                std::cout << resultado.second[i];
                if (i < resultado.second.size() - 1) {
                    std::cout << " => ";
                }
            }
            break;
        case REJEITADA:
            std::cout << "rejeitada";
            break;
    }
    std::cout << "\n";
}
// uma gracinha que quisemos fazer para enfeitar :)
void exibir_banner() {
    std::cout << R"(
 

     _     _      _____    ____                           
    | |   | |    / / \ \  |  _ \ __ _ _ __ ___  ___ _ __  
    | |   | |   | || || | | |_) / _` | '__/ __|/ _ \ '__| 
    | |___| |___| || || | |  __/ (_| | |  \__ \  __/ |    
    |_____|_____| ||_|| | |_|   \__,_|_|  |___/\___|_|    
                 \_\  /_/                                  
                                                        
    ╔═══════════════════════════════════════════════════════════╗
    ║               Analisador Sintático LL(1)                  ║
    ║                      20-11-2025                           ║
    ╚═══════════════════════════════════════════════════════════╝
)" << "\n";
}

int main(int argc, char const *argv[]) {
    using std::cout;
    
    exibir_banner();
    
    if (argc != 2) {
        cout << "Uso:\n"
             << "  " << argv[0] << " <caminho para arquivo de gramática>\n\n"
             << "Exemplo:\n"
             << "  " << argv[0] << " gramatica.txt\n";
        return EXIT_FAILURE;
    }

    // analisando o arquivo de gramática
    std::ifstream arquivo_gramatica{argv[1], std::ios::in};
    if (arquivo_gramatica.fail()) {
        cout << "Erro ao abrir o arquivo de gramática\n";
        return EXIT_FAILURE;
    }
    
    Gramatica gram{ler_arquivo(arquivo_gramatica)};
    cout << "Gramática analisada com sucesso:\n\n" << gram << "\n";
    
    cout << "═══════════════════════════════════════════\n";
    cout << "  Modo Interativo - Digite suas strings\n";
    cout << "  (Pressione Ctrl+D para finalizar)\n";
    cout << "═══════════════════════════════════════════\n";
    
    while (true) {
        std::string texto;
        cout << "➤ ";
        if (!std::getline(std::cin, texto)) {
            break;
        }
        mostrar_resultado(texto, Validador::validar_entrada(texto, gram));
    }
    
    cout << "\n✓ Programa finalizado.\n";
    return EXIT_SUCCESS;
}
