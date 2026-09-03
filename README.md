# Documentação do Compilador JIT: PicoQuickProcessor (PQP) para x86_64

## 1. Introdução

Este documento detalha a arquitetura, o design e as decisões de implementação por trás do emulador e compilador *Just-In-Time* (JIT) para o **PicoQuickProcessor (PQP)**. O objetivo deste projeto foi traduzir dinamicamente as instruções de uma arquitetura simulada (PQP) diretamente para o código de máquina nativo da arquitetura hospedeira (x86_64), extraindo o máximo de performance possível.

Para alcançar a velocidade de execução nativa, o projeto abandona as abordagens tradicionais de interpretação em software e adota técnicas agressivas de injeção de código, manipulação de contexto via sinais do sistema operacional (`SIGTRAP`) e uso avançado de extensões vetoriais (AVX/SSE).

## 2. O que é o PQP (PicoQuickProcessor)?

O PQP é uma arquitetura hipotética e simplificada, criada pelo professor Bruno Otávio Piedade Prado (Departamento de Computação / UFS), desenhada com características clássicas de máquinas RISC. Suas principais restrições e propriedades incluem:

* **16 Registradores de Propósito Geral (GPRs):** Numerados de `r0` a `r15`.

* **Memória Von Neumann Microscópica:** Apenas 256 bytes de espaço de endereçamento total, contendo dados e instruções.

* **Arquitetura Little-Endian de 32 bits:** Todas as operações nativas lidam com inteiros de 32 bits, com saltos calculados com offsets e imediatos estendidos por sinal.

* **Conjunto de Instruções Simples:** Inclui operações aritméticas básicas (`ADD`, `SUB`, `AND`, `OR`, `XOR`, `SAL`, `SAR`), movimentação de dados (`MOV`) e controle de fluxo (`JMP`, `JG`, `JL`, `JE`).

### 2.1 Especificação do Conjunto de Instruções (ISA)

A tabela abaixo define os *opcodes*, operandos e as operações lógicas/matemáticas da arquitetura de 32-bits do PQP:

| Instrução | Opcode (Hex) | Semântica | Operação Exata | 
| ----- | ----- | ----- | ----- | 
| `mov rx, i16` | `0x00` | Move imediato de 16 bits (estendido com sinal) para `rx` | `rx = i16` | 
| `mov rx, ry` | `0x01` | Move o valor do registrador `ry` para `rx` | `rx = ry` | 
| `mov rx, [ry]` | `0x02` | Carrega em `rx` o valor da memória apontada por `ry` | `rx = MEM[ry]` | 
| `mov [rx], ry` | `0x03` | Armazena na memória apontada por `rx` o valor de `ry` | `MEM[rx] = ry` | 
| `cmp rx, ry` | `0x04` | Compara `rx` e `ry` (atualiza as flags lógicas) | `rx <-> ry` | 
| `jmp i16` | `0x05` | Salto incondicional relativo | `pc += 4 + i16` | 
| `jg i16` | `0x06` | Salto relativo se maior (*Greater*) | `se (>) pc += 4 + i16` | 
| `jl i16` | `0x07` | Salto relativo se menor (*Less*) | `se (<) pc += 4 + i16` | 
| `je i16` | `0x08` | Salto relativo se igual (*Equal*) | `se (==) pc += 4 + i16` | 
| `add rx, ry` | `0x09` | Adição | `rx += ry` | 
| `sub rx, ry` | `0x0A` | Subtração | `rx -= ry` | 
| `and rx, ry` | `0x0B` | AND lógico bit a bit | `rx &= ry` | 
| `or rx, ry` | `0x0C` | OR lógico bit a bit | `rx |= ry` | 
| `xor rx, ry` | `0x0D` | XOR lógico bit a bit | `rx ^= ry` | 
| `sal rx, i5` | `0x0E` | Deslocamento aritmético para a esquerda (*Shift Left*) | `rx <<= i5` | 
| `sar rx, i5` | `0x0F` | Deslocamento aritmético para a direita (*Shift Right*) | `rx >>= i5` | 

## 3. Estratégias e Ideias Criativas da Implementação

Criar um JIT de alto desempenho exige pensar fora da caixa, especialmente ao contornar as limitações físicas da arquitetura x86_64. Abaixo estão as ideias mais criativas que implementamos e suas implicações.

### 3.1 Mapeamento 1:1 de Registradores

Ao invés de simular os 16 registradores do PQP em um array na memória RAM (como o QEMU faz em sua estrutura genérica), nós mapeamos os registradores do PQP **diretamente para os 16 GPRs físicos do x86_64** (`%rax` até `%r15`).

* **A Ideia:** Instruções como `ADD r1, r2` no PQP são traduzidas para um simples `ADD %rcx, %rdx` em x86_64 (1 byte vs várias operações de load/store).

* **Implicações:** Ganhamos uma velocidade absurda, pois as operações ocorrem diretamente no silício da CPU hospedeira. Contudo, isso "sequestra" todos os registradores do hospedeiro, incluindo o ponteiro de pilha (`%rsp`). Isso nos forçou a criar um **Trampolim em Assembly** para salvar o contexto do C++ antes de pular para o código JIT, e usar uma pilha alternativa de sinais (`sigaltstack`) para garantir que o Kernel pudesse interromper o programa em segurança.

### 3.2 O Guest PC Isolado nos Registradores Vetoriais (XMM)

Com todos os 16 GPRs do hospedeiro ocupados pelos dados do PQP, não havia espaço para armazenar o Contador de Programa (Guest PC).

* **A Ideia:** Decidimos isolar o Guest PC no registrador vetorial `%xmm1` (em algumas iterações abstraído em registradores altos), usando instruções da família SSE/AVX (`vmovd`).

* **Implicações:** Essa foi uma das sacadas mais importantes do projeto. As instruções matemáticas que operam nos registradores XMM **não afetam a EFLAGS** da CPU. Se atualizássemos o PC usando um registrador comum, destruiríamos o resultado de instruções `CMP` executadas antes de saltos condicionais. Mantendo o PC seguro em um registrador SIMD, o fluxo de controle e a matemática do PQP seguem intactos.

### 3.3 Geração de Blocos Básicos e o "Block Chaining"

O nosso JIT não traduz o programa inteiro de uma vez (como um compilador *Ahead-Of-Time*) nem decodifica instrução por instrução durante a execução (como um interpretador puro). A unidade de compilação é o **Bloco Básico** — uma sequência linear de instruções que só possui um ponto de entrada e sempre termina em uma instrução de desvio de fluxo (um salto incondicional, um salto condicional ou um *halt*).

Para garantir a execução fluida e a auto-otimização, cada bloco gerado em memória nativa possui três partes anatômicas distintas: o *Header*, o *Body* e o *Footer*.

#### A Anatomia do Bloco

1. **O Header (Cabeçalho de Profiling):**
   A primeira coisa que um bloco recém-gerado faz ao ser executado é contar a si mesmo. Injetamos no início do bloco instruções da extensão vetorial (AVX) para ler o contador do bloco na memória, incrementá-lo e salvá-lo novamente (usando `vmovd` e `vpaddd` com `%xmm0` e `%xmm2`).

   * *Por que AVX?* Porque as instruções matemáticas em registradores XMM não sujam a EFLAGS (as flags de *carry*, *zero*, *overflow* da CPU hospedeira). Se usássemos um simples `INC [mem]`, poderíamos destruir um estado condicional pendente que entrou no bloco, o que corromperia o programa.

2. **O Body (Corpo das Instruções):**
   Esta é a "carne" do bloco. O JIT varre a memória PQP decodificando sequencialmente e emitindo os bytes x86_64 correspondentes para as operações lógicas, matemáticas e de movimentação de memória (`emmit_MOV`, `emmit_BINARY_OP`). Aqui a execução "voa", rodando mapeada 1:1 diretamente nos registradores nativos.

3. **O Footer (O Rodapé de Desvio e os Stubs):**
   Quando o JIT encontra uma instrução de controle de fluxo (um `JMP` ou `Jcc` do PQP), o bloco acaba. O papel do *Footer* é preparar o terreno para sair do bloco atual e ir para o próximo. Para isso, o Footer executa duas tarefas cruciais:

   * **Update Guest PC:** Ele escreve no `%xmm1` o endereço simulado (PC) da próxima instrução.

   * **O Placeholder de Interrupção (`0xCC`):** Como o JIT frequentemente não sabe onde o bloco de destino está na memória nativa (ou se sequer foi compilado ainda), nós emitimos um byte de *Hardware Trap* (`0xCC` / `INT3`), seguido de 4 bytes falsos (`0x90909090`).

#### O Desafio dos Saltos Condicionais (JMPS Condicionais)

O controle de fluxo incondicional (`JMP`) é fácil: ele tem apenas uma saída. Mas um salto condicional do PQP (`JG`, `JL`, `JE`) ramifica a execução em dois caminhos possíveis:

* O caminho do **Target** (Se a condição for verdadeira).

* O caminho de **Fallthrough** (Se a condição for falsa, e o código apenas prosseguir para `PC + 4`).

Para lidar com isso, o *Footer* condicional é emitido com uma engenharia muito específica:

1. **A bifurcação nativa (`0x0F 0x8X`):** O compilador emite a instrução de *Jump Conditional* genuína do x86 (ex: se for `JE` no PQP, emite um `JE` longo no x86). O offset desse salto nativo é fixado *hardcoded* para avançar exatamente **14 bytes** pra frente se for ativado.

2. **O Stub do Fallthrough:** Imediatamente após o salto condicional nativo (o caminho caso a condição falhe), emitimos um mini-bloco que atualiza o `%xmm1` com `PC + 4` e executa um `0xCC` (Trap).

3. **O Stub do Target:** Exatamente 14 bytes depois (o destino do nosso salto nativo), começa o segundo mini-bloco. Ele atualiza o `%xmm1` com `Target_PC` e executa outro `0xCC`.

*(A lógica é bela: o salto x86 literalmente "pula por cima" do Stub de Fallthrough para cair direto no Stub do Target caso a flag sinalize positivo).*

#### A Mágica do "Block Chaining" (Remendo Dinâmico)

A primeira vez que um bloco chega no final, ele invariavelmente bate no nosso `0xCC`. Isso gera um sinal de `SIGTRAP`. O Kernel congela a thread x86 e joga a execução para o nosso tratador C++ (`jit_handler`).

É aqui que a genialidade acontece:

1. O C++ lê no registrador `%xmm1` do contexto capturado para onde o programa PQP queria ir.

2. Ele compila o novo bloco alvo (se ainda não existir).

3. E, finalmente, **ele apaga o `0xCC` da memória física** onde o programa parou, substituindo-o por um `0xE9` (JMP Relativo x86 puro de 32 bits), apontando fisicamente a distância matemática daquele antigo final para o recém-gerado cabeçalho do novo bloco.

**Conclusão desta técnica:** O `SIGTRAP` e o custo de ir para o C++ só acontecem **uma única vez por aresta de grafo de fluxo de controle**. Da segunda iteração em diante (como num loop pesado), a CPU hospedeira apenas lê as instruções x86, faz os cálculos, e os antigos `0xCC` agora são `JMPs` diretos. Os blocos "voam" entre si no silício a milhões de iterações por segundo, justificando a altíssima performance do JIT frente aos interpretadores clássicos.

### 3.4 Profiling de Alta Performance via AVX2

Como visto acima na composição do *Header*, o profiling é feito diretamente no silício. Como garantimos a integridade matemática dos blocos básicos, basta, no fim da execução, ler a "Block Metadata Table" e transpor o valor daquele contador vetorial injetado no Header para todas as instruções internas que faziam parte daquele bloco. Isso garante um log (`pqp_golden.output.txt`) preciso, mapeado endereço a endereço, com *overhead* de tempo de execução quase nulo, pois não existe chaveamento de contexto ou ifs em C++ para incrementar contadores.

### 3.5 Dominando o Byte ModR/M, Prefixo REX e Memória (MAP_32BIT)

A codificação direta de x86 exige precisão com a extensão de 64 bits.

* **A Ideia:** Para que as instruções vetoriais pudessem ler a memória contendo o Guest PC e os contadores rapidamente sem usar registradores de rascunho (pois não havia nenhum sobrando), obrigamos o `mmap` a alocar nossas *Arenas de Constantes* usando a flag `MAP_32BIT`.

* **Implicações:** Isso garante que os endereços fiquem restritos ao limite de 4GB, permitindo injetar o ponteiro absoluto diretamente no código de máquina usando *disp32* (`0x66 0x0F 0x6E 0x14 0x25 [ADDR]`). Além disso, manipulamos com maestria os bits REX (`0x41`, `0x44`, `0x45`) e máscaras lógicas (`& 0x07`) no C++ para lidar com a rotação reversa dos opcodes de *Load* (`0x8B`) e *Store* (`0x89`).

## 4. Notas sobre a Documentação de Registradores

Para referências e aprofundamento técnico, **a documentação base referente ao uso e decodificação dos registradores de uso geral (GPRs) da arquitetura x86_64 está localizada na pasta `docs` do projeto**.

*Nota de Desenvolvimento Futuro:* Esta seção será futuramente incrementada com os detalhes lógicos da arquitetura de **registradores virtuais**, documentando como o motor gerencia o *register spilling* e o pipeline de tradução em cenários onde a arquitetura hospedeira apresente menos registradores que a máquina virtualizada.

## 5. Conclusão

O JIT do PicoQuickProcessor não é apenas um projeto de tradução binária, mas um laboratório avançado de interações diretas com o Kernel via `ucontext_t` e manipulação cirúrgica de contexto de hardware. Ele prova que é possível escrever código auto-modificável dinâmico e seguro e superar amplamente a velocidade dos interpretadores tradicionais explorando os "atalhos" e peculiaridades do silício hospedeiro.
