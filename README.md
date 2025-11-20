# Analisador Sintático LL(1)

Um analisador sintatico ou parser LL(1) implementado em C++ que processa gramáticas livres de contexto e valida strings de entrada.

## Características

- Cálculo automático dos conjuntos PRIMEIRO e SEGUIDOR
- Construção da tabela de análise sintática LL(1)
- Validação de strings com exibição da derivação completa
- Interface interativa para testar múltiplas entradas

## Requisitos

- Compilador C++ com suporte ao padrão C++20 (g++ 10+, clang++ 10+)
- Sistema operacional: Linux, macOS ou Windows (com MinGW/WSL)

## Compilação

```bash
g++ -std=c++20 parser_ll1_codigo.cpp -o parser_compilado
```

## Formato do Arquivo de Gramática

Crie um arquivo de texto com as regras de produção, uma por linha, no formato:

```
X→αβγ
```

Onde:
- `X` é um não-terminal (letra maiúscula)
- `→` separa o lado esquerdo do direito
- `αβγ` é o lado direito da produção
- Use `e` para representar ε (epsilon/vazio)
- Delimitadores `()[]{}<>` são permitidos mas ignorados

**Exemplo** (`gramatica.txt`):
```
S→(A)
A→aAb
A→e
```

## Execução

```bash
./parser_compilado gramatica.txt
```

O programa irá:
1. Carregar e analisar a gramática
2. Exibir os conjuntos PRIMEIRO e SEGUIDOR
3. Mostrar a tabela de análise sintática
4. Entrar no modo interativo

## Modo Interativo

Digite strings para validar:

```
➤ aabb
[aabb] aceita. Derivação: S ⇒ (A) ⇒ (aAb) ⇒ (aabb)

➤ ab
[ab] rejeitada

➤ xyz
[xyz] contém símbolos desconhecidos
```

Para sair, pressione `Ctrl+D` (Linux/macOS) ou `Ctrl+Z` + Enter (Windows).

## Exemplo Completo

```bash
# 1. Criar arquivo de gramática
echo -e "S→(A)\nA→aAb\nA→e" > gramatica.txt

# 2. Compilar
g++ -std=c++20 parser_ll1_codigo.cpp -o parser_compilado

# 3. Executar
./parser_compilado gramatica.txt
```

- Conflitos na tabela de análise indicam que a gramática não é LL(1)
