#include "config.hpp"
#include "controller.hpp"
#include "support.hpp"

#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace cg;
namespace {
void require(bool value, const std::string& message) {
    if (!value) throw std::runtime_error(message);
}
void near(double a, double b, double tolerance = 1e-7) {
    if (std::abs(a - b) > tolerance)
        throw std::runtime_error("Esperado " + std::to_string(b) + ", recebido " + std::to_string(a));
}
Config config() { Config c; c.fixedSeed = true; c.seed = 42; return c; }
Game emptyGame(Config c = config()) { Game g(c); GameTestAccess::empty(g); return g; }
std::string validXml() {
    return R"(<jogo><arena largura="640" altura="720"/>
<jogador raioCabeca="20" velocidade="123" velocidadeGiro="90" velocidadeTiro="456"/>
<inimigo raioCabeca="12" tirosPorSegundo="2.5" velocidadeTiro="234"/>
<barril largura="80" altura="60" numeroTiros="4" nParaGanhar="7" velocidade="45"/>
<outros intervaloBarris="1.25" sensibilidadeMouse="0.5" semente="123"/></jogo>)";
}
std::string replace(std::string text, const std::string& from, const std::string& to) {
    const auto i = text.find(from);
    require(i != std::string::npos, "Fixture XML nao encontrada");
    text.replace(i, from.size(), to); return text;
}
Config parse(const std::string& xml) {
    const auto path = std::filesystem::temp_directory_path() / "cg-test-config.xml";
    { std::ofstream f(path); f << xml; }
    try { const Config c = Config::load(path); std::filesystem::remove(path); return c; }
    catch (...) { std::filesystem::remove(path); throw; }
}
void rejects(const std::string& xml) {
    bool rejected = false;
    try { parse(xml); } catch (const std::exception&) { rejected = true; }
    require(rejected, "XML invalido foi aceito");
}
void bulletAtBarrel(Game& g, std::size_t index = 0) {
    GameTestAccess::addBullet(g, g.barrels().at(index).position, {0,0});
    g.advance(Game::timeStep);
}
}

int main() {
    int passed = 0, failed = 0;
    auto test = [&](const std::string& name, const std::function<void()>& run) {
        try { run(); ++passed; std::cout << "OK  " << name << '\n'; }
        catch (const std::exception& error) { ++failed; std::cerr << "FALHA  " << name << ": " << error.what() << '\n'; }
    };
    test("XML: todos os parametros sao lidos", [] {
        const auto c = parse(validXml());
        require(c.arenaWidth == 640 && c.arenaHeight == 720, "Arena");
        near(c.playerRadius,20); near(c.playerSpeed,123); near(c.turnSpeed,90); near(c.playerShotSpeed,456);
        near(c.enemyRadius,12); near(c.enemyShotSpeed,234); near(c.enemyShotsPerSecond,2.5);
        near(c.barrelWidth,80); near(c.barrelHeight,60); near(c.barrelSpeed,45);
        require(c.barrelHitPoints == 4 && c.barrelsToWin == 7, "Resistencia/meta");
        near(c.spawnInterval,1.25); near(c.mouseSensitivity,0.5);
        require(c.fixedSeed && c.seed == 123, "Semente");
    });
    test("XML: formato original sem extras opcionais", [] {
        const auto c = parse(R"(<jogo><arena altura="500" largura="200"/>
<jogador raioCabeca="20" velocidade="50"/><inimigo raioCabeca="15" tirosPorSegundo="1" velocidadeTiro="120"/>
<barril altura="40" largura="50" numeroTiros="3" nParaGanhar="5" velocidade="20"/><outros pensar="..."/></jogo>)");
        near(c.playerShotSpeed,120); require(!c.fixedSeed,"Aleatoriedade padrao");
    });
    test("XML: erros sintaticos, raiz e elementos obrigatorios", [] {
        for (const auto& xml : {"", "<jogo>", "<foo/>", "<jogo/><jogo/>", "<jogo/>"}) rejects(xml);
        rejects(replace(validXml(), "<arena largura=\"640\" altura=\"720\"/>", ""));
        rejects(replace(validXml(), "</jogo>", "<arena largura=\"100\" altura=\"200\"/></jogo>"));
    });
    test("XML: valores invalidos e limites geometricos", [] {
        for (const auto& bad : {"...", "nan", "inf", "-1", "0", "20px", ""})
            rejects(replace(validXml(), "raioCabeca=\"20\"", "raioCabeca=\"" + std::string(bad) + "\""));
        rejects(replace(validXml(), "numeroTiros=\"4\"", "numeroTiros=\"1.5\""));
        rejects(replace(validXml(), "nParaGanhar=\"7\"", "nParaGanhar=\"0\""));
        rejects(replace(validXml(), "largura=\"80\"", "largura=\"1000\""));
        rejects(replace(validXml(), "raioCabeca=\"20\"", "raioCabeca=\"181\""));
        rejects(replace(validXml(), "largura=\"640\"", "largura=\"640.5\""));
        rejects(replace(validXml(), "semente=\"123\"", "semente=\"-1\""));
        rejects(replace(validXml(), "semente=\"123\"", "semente=\"4294967296\""));
        rejects(replace(validXml(), "intervaloBarris=\"1.25\"", "intervaloBarris=\"0\""));
    });
    test("XML: arquivo inexistente retorna erro", [] {
        bool caught = false;
        try { Config::load("/tmp/cg-arquivo-inexistente-829732.xml"); } catch (...) { caught = true; }
        require(caught, "Arquivo inexistente");
    });
    test("Inicio: jogador no centro da base e arena superior livre", [] {
        const Game g(config()); const auto& c=g.config();
        near(g.player().position.x,c.arenaWidth/2.0);
        require(g.player().position.y >= c.playerRadius && g.player().position.y < c.arenaHeight/4.0,"Base");
        near(g.player().angle,0); require(g.destroyed()==0 && g.status()==Status::Playing,"Estado inicial");
    });
    test("W: andar na direcao frontal com velocidade configurada", [] {
        auto g=emptyGame(); const Vec2 before=g.player().position;
        g.setKey('w',true); g.advance(0.5);
        near(g.player().position.x,before.x); near(g.player().position.y,before.y+90);
    });
    test("S: andar para tras conforme orientacao", [] {
        auto g=emptyGame(); GameTestAccess::player(g).position={300,250}; GameTestAccess::player(g).angle=90;
        g.setKey('s',true); g.advance(0.5); near(g.player().position.x,390); near(g.player().position.y,250);
    });
    test("A/D: sinais de rotacao e nenhuma translacao", [] {
        auto g=emptyGame(); const Vec2 before=g.player().position;
        g.setKey('a',true); g.advance(0.5); near(g.player().angle,75);
        g.setKey('a',false); g.setKey('d',true); g.advance(1); near(g.player().angle,-75);
        near(length(g.player().position-before),0); near(g.player().walkPhase,0);
    });
    test("W+A e W+D: arcos de circulo corretos", [] {
        for (const char key : {'a','d'}) {
            auto g=emptyGame(); GameTestAccess::player(g).position={300,200};
            g.setKey('w',true); g.setKey(key,true); g.advance(0.4);
            const double sign=key=='a'?1:-1, omega=radians(sign*150);
            near(g.player().position.x,300+180/omega*(std::cos(omega*0.4)-1));
            near(g.player().position.y,200+180/omega*std::sin(omega*0.4));
        }
    });
    test("Teclas opostas se cancelam e maiusculas funcionam", [] {
        auto g=emptyGame(); const auto p=g.player();
        for (char k : {'W','S','A','D'}) g.setKey(k,true);
        g.advance(1); near(length(g.player().position-p.position),0); near(g.player().angle,0);
        g.setKey('S',false); g.advance(0.5); near(g.player().position.y,p.position.y+90);
    });
    test("KeyUp: soltar tecla interrompe movimento", [] {
        auto g=emptyGame(); Controller input(g); input.keyDown('w'); g.advance(0.2);
        input.keyUp('w'); const auto p=g.player().position; g.advance(0.5); near(length(g.player().position-p),0);
    });
    test("Pernas: animam para frente/tras e param em repouso", [] {
        for (char k : {'w','s'}) {
            auto g=emptyGame(); GameTestAccess::player(g).position={300,200};
            g.setKey(k,true); g.advance(0.1); require(std::abs(g.player().walkPhase)>0.1,"Passada");
            g.setKey(k,false); g.advance(0.1); near(g.player().walkPhase,0);
        }
    });
    test("Cabeca: limites esquerdo, direito, base e linha central", [] {
        for (double angle : {0.0,90.0,180.0,-90.0}) {
            auto g=emptyGame(); GameTestAccess::player(g).angle=angle;
            g.setKey('w',true); g.advance(10);
            const auto p=g.player(); const auto c=g.config();
            require(p.position.x>=p.radius && p.position.x<=c.arenaWidth-p.radius,"Limite X");
            require(p.position.y>=p.radius && p.position.y<=c.arenaHeight/2.0-p.radius,"Limite Y");
            if (angle==0) near(p.position.y,c.arenaHeight/2.0-p.radius);
            if (angle==90) near(p.position.x,p.radius);
            if (angle==180) near(p.position.y,p.radius);
            if (angle==-90) near(p.position.x,c.arenaWidth-p.radius);
        }
    });
    test("Escala: pivô, cano e partes acompanham raio e corpo", [] {
        Character a{{0,0},10,47,17,0}, b{{0,0},20,47,17,0};
        near(length(b.gunPivot()-a.gunPivot()*2),0); near(length(b.muzzle()-a.muzzle()*2),0);
        b.position={80,90}; near(length(b.muzzle()-a.muzzle()*2-b.position),0);
    });
    test("Mouse: esquerda anti-horario, direita horario, limite total 60 graus", [] {
        auto g=emptyGame(); Controller input(g); input.motion(300); near(g.player().gunAngle,0);
        input.motion(260); near(g.player().gunAngle,10); input.motion(300); near(g.player().gunAngle,0);
        input.motion(0); near(g.player().gunAngle,30); input.motion(1000); near(g.player().gunAngle,-30);
        near(g.player().angle,0);
    });
    test("Clique: um tiro por pressao; segurar/soltar nao dispara", [] {
        auto g=emptyGame(); Controller input(g);
        input.mouseLeft(true,300); input.mouseLeft(true,300); require(g.bullets().size()==1,"Botao mantido");
        input.mouseLeft(false,300); require(g.bullets().size()==1,"Soltar");
        input.mouseLeft(true,300); require(g.bullets().size()==2,"Novo clique");
    });
    test("Tiro: nasce na ponta da arma e usa sua orientacao", [] {
        auto g=emptyGame(); auto& p=GameTestAccess::player(g); p.angle=37;p.gunAngle=19;
        g.fire(); const auto b=g.bullets().at(0);
        near(length(b.position-p.muzzle()),0); near(length(b.velocity-forward(56)*480),0);
    });
    test("Tiro: trajeto independente apos jogador andar e girar", [] {
        auto g=emptyGame(); g.fire(); const auto b=g.bullets().at(0);
        g.setKey('w',true);g.setKey('a',true);g.aim(50);g.advance(0.3);
        near(length(g.bullets().at(0).position-(b.position+b.velocity*0.3)),0);
        near(length(g.bullets().at(0).velocity-b.velocity),0);
    });
    test("Tiros deixam arena e sao removidos", [] {
        auto g=emptyGame();g.fire();g.advance(5);require(g.bullets().empty(),"Limpeza");
    });
    test("Circulo-retangulo: contato, tangencia e canto sem falso positivo", [] {
        const Rect r{{0,0},20,20};
        require(circleRect({15,0},5,r),"Tangencia lateral");require(circleRect({0,0},1,r),"Interior");
        require(!circleRect({14,14},5,r),"Canto externo");require(circleRect({13,14},5,r),"Canto tangente");
        require(!std::isfinite(sweepCircleRect({14,14},{14,14},5,r)),"Canto varrido externo");
        near(sweepCircleRect({-30,0},{30,0},5,r),0.25);
    });
    test("Colisao: barril tocando a cabeca encerra com derrota", [] {
        auto g=emptyGame(); GameTestAccess::player(g).position={300,200};
        GameTestAccess::addBarrel(g,{300+38+18,200},3);g.toggleDemo(2);g.advance(Game::timeStep);
        require(g.status()==Status::Lost && g.lossReason()==LossReason::Barrel,"Derrota por contato");
    });
    test("Colisao: arma e pernas nao contam como cabeca", [] {
        auto c=config(); c.barrelHeight=10;c.barrelWidth=10;c.barrelSpeed=0;
        auto g=emptyGame(c);GameTestAccess::player(g).position={300,200};
        GameTestAccess::addBarrel(g,g.player().muzzle(),3);g.advance(Game::timeStep);
        require(g.status()==Status::Playing,"Colisao deve usar cabeca");
    });
    test("Colisao continua: barril rapido nao atravessa jogador", [] {
        auto c=config();c.barrelSpeed=10000;c.barrelHeight=2;c.barrelWidth=20;c.playerRadius=2;
        auto g=emptyGame(c);GameTestAccess::player(g).position={300,200};
        GameTestAccess::addBarrel(g,{300,220},3);g.advance(Game::timeStep);
        require(g.status()==Status::Lost,"Barril atravessou");
    });
    test("Colisao: tiro inimigo mata; tiro proprio nao mata", [] {
        auto g=emptyGame();GameTestAccess::addBullet(g,g.player().position,{0,0});g.advance(Game::timeStep);
        require(g.status()==Status::Playing,"Tiro proprio");
        GameTestAccess::addBullet(g,g.player().position,{0,0},Owner::Enemy);g.advance(Game::timeStep);
        require(g.status()==Status::Lost && g.lossReason()==LossReason::Shot,"Tiro inimigo");
    });
    test("Colisao continua: tiro inimigo rapido e jogador em movimento", [] {
        auto g=emptyGame();GameTestAccess::player(g).position={300,200};g.setKey('w',true);
        GameTestAccess::addBullet(g,{300,400},{0,-100000},Owner::Enemy);g.advance(Game::timeStep);
        require(g.status()==Status::Lost,"Tiro rapido atravessou");
    });
    test("Resistencia: cada tiro tira exatamente uma vida", [] {
        auto g=emptyGame();GameTestAccess::addBarrel(g,{300,500},3);
        bulletAtBarrel(g);require(g.barrels().at(0).hitPoints==2 && g.destroyed()==0,"Primeiro tiro");
        bulletAtBarrel(g);require(g.barrels().at(0).hitPoints==1 && g.destroyed()==0,"Segundo tiro");
        bulletAtBarrel(g);require(g.barrels().empty() && g.destroyed()==1,"Terceiro tiro");
    });
    test("Um projetil atinge somente o primeiro barril do trajeto", [] {
        auto g=emptyGame();GameTestAccess::addBarrel(g,{300,500},3);GameTestAccess::addBarrel(g,{300,300},3);
        GameTestAccess::addBullet(g,{300,100},{0,100000});g.advance(Game::timeStep);
        require(g.barrels().at(0).hitPoints==3 && g.barrels().at(1).hitPoints==2,"Ordem espacial");
        require(g.bullets().empty(),"Consumo do tiro");
    });
    test("Varios tiros simultaneos nao duplicam a pontuacao", [] {
        auto g=emptyGame();GameTestAccess::addBarrel(g,{300,500},1,true);
        for(int i=0;i<4;++i)GameTestAccess::addBullet(g,{300,500},{0,0});
        g.advance(Game::timeStep);require(g.destroyed()==1 && g.barrels().empty(),"Pontuacao unica");
    });
    test("Tiros inimigos nao retiram resistencia dos barris", [] {
        auto g=emptyGame();GameTestAccess::addBarrel(g,{300,500},3);
        GameTestAccess::addBullet(g,{300,500},{0,0},Owner::Enemy);g.advance(Game::timeStep);
        require(g.barrels().at(0).hitPoints==3,"Fogo aliado inimigo");
    });
    test("Destruir N barris conclui a fase sem encerrar a campanha", [] {
        auto c=config();c.barrelsToWin=2;auto g=emptyGame(c);
        for(int i=0;i<2;++i) {
            GameTestAccess::addBarrel(g,{300,500},1);bulletAtBarrel(g);
            require(g.destroyed()==i+1,"Placar");require(g.status()==Status::Playing,"Campanha continua");
            require(g.transitioning()==(i==1),"Meta da fase");
        }
    });
    test("Fim de jogo bloqueia tiros, movimento, mira e tempo", [] {
        for(auto status:{Status::Won,Status::Lost}) {
            auto g=emptyGame();GameTestAccess::terminal(g,status);const auto p=g.player();
            g.setKey('w',true);g.fire();g.aim(80);g.advance(1);
            near(length(g.player().position-p.position),0);near(g.player().gunAngle,p.gunAngle);
            near(g.elapsed(),0);require(g.bullets().empty(),"Tiros apos fim");
        }
    });
    test("R: reinicia apos derrota e restaura demo, teclas e placar", [] {
        auto g=emptyGame();Controller input(g);g.toggleDemo(2);g.toggleDemo(3);g.toggleDemo(5);
        GameTestAccess::terminal(g,Status::Lost);input.keyDown('r');
        require(g.status()==Status::Playing && g.destroyed()==0 && g.bullets().empty(),"Estado resetado");
        require(g.demo().barrelMotion && g.demo().enemyFire && g.demo().spawning && !g.demo().hitboxes,"Demo resetado");
        require(g.barrels().size()==1,"Primeiro barril");near(g.elapsed(),0);
    });
    test("Barris nascem no topo, com dimensoes e resistencia configuradas", [] {
        Game g(config());
        for(int i=0;i<100;++i) {
            GameTestAccess::barrels(g).clear();GameTestAccess::spawn(g);const auto b=g.barrels().at(0);const auto c=g.config();
            near(b.position.y,c.arenaHeight-c.barrelHeight/2);require(b.position.x>=c.barrelWidth/2 && b.position.x<=c.arenaWidth-c.barrelWidth/2,"X dentro");
            require(b.hitPoints==c.barrelHitPoints,"Resistencia");
        }
    });
    test("Sorteio: posicoes variadas e inimigo com probabilidade de 50%", [] {
        Game g(config());int enemies=0;double minX=10000,maxX=-1;
        for(int i=0;i<10000;++i) {
            GameTestAccess::barrels(g).clear();GameTestAccess::spawn(g);const auto b=g.barrels().at(0);
            enemies+=b.hasEnemy;minX=std::min(minX,b.position.x);maxX=std::max(maxX,b.position.x);
        }
        require(enemies>4700 && enemies<5300,"Distribuicao de inimigos");require(maxX-minX>450,"Distribuicao horizontal");
    });
    test("Cabecas grandes dos inimigos continuam inteiras na arena", [] {
        auto c=config();c.enemyRadius=90;Game g(c);
        for(int i=0;i<200;++i) {
            GameTestAccess::barrels(g).clear();GameTestAccess::spawn(g);const auto b=g.barrels().at(0);
            if(b.hasEnemy)require(b.position.x>=90 && b.position.x<=510 && b.position.y<=710,"Cabeca fora");
        }
    });
    test("Barris rolam em linha reta na velocidade do XML", [] {
        auto g=emptyGame();GameTestAccess::addBarrel(g,{80,600},3);g.advance(2);
        near(g.barrels().at(0).position.x,80);near(g.barrels().at(0).position.y,470);
        require(g.barrels().at(0).rollPhase!=0,"Animacao rolamento");
    });
    test("Barris perdidos saem da arena sem pontuar", [] {
        auto g=emptyGame();GameTestAccess::addBarrel(g,{80,100},3);g.advance(3);
        require(g.barrels().empty() && g.destroyed()==0 && g.status()==Status::Playing,"Barril perdido");
    });
    test("Intervalo de criacao usa segundos", [] {
        auto c=config();c.spawnInterval=0.5;c.barrelSpeed=0;c.enemyShotsPerSecond=0;Game g(c);g.advance(2);
        require(g.barrels().size()==5,"Barril inicial + quatro nascimentos");
    });
    test("Oponente acompanha barril e sempre anima as pernas", [] {
        auto g=emptyGame();GameTestAccess::addBarrel(g,{100,600},3,true);g.advance(0.1);
        const auto b=g.barrels().at(0);near(length(b.enemy.position-b.position),0);require(b.enemy.walkPhase>0,"Andar inimigo");
        g.toggleDemo(2);const double phase=b.enemy.walkPhase;g.advance(0.1);
        near(g.barrels().at(0).position.y,b.position.y);require(g.barrels().at(0).enemy.walkPhase!=phase,"Pernas sempre animadas");
    });
    test("Arma inimiga aponta para jogador a partir do proprio pivo", [] {
        auto g=emptyGame();GameTestAccess::addBarrel(g,{300,600},3,true);
        const auto e=g.barrels().at(0).enemy;const auto target=g.player().position-e.gunPivot();
        const auto direction=forward(e.angle+e.gunAngle);
        near(target.x*direction.y-target.y*direction.x,0);require(dot(target,direction)>0,"Sentido da mira");
    });
    test("Oponente respeita limite de mira em ambos os lados", [] {
        auto g=emptyGame();GameTestAccess::addBarrel(g,{300,210},3,true);
        GameTestAccess::player(g).position={20,200};GameTestAccess::aimEnemy(g);near(g.barrels().at(0).enemy.gunAngle,-30);
        GameTestAccess::player(g).position={580,200};GameTestAccess::aimEnemy(g);near(g.barrels().at(0).enemy.gunAngle,30);
    });
    test("Tiros inimigos: frequencia fracionaria, velocidade e origem", [] {
        auto c=config();c.enemyShotsPerSecond=2.5;c.enemyShotSpeed=10;c.barrelSpeed=0;
        auto g=emptyGame(c);g.toggleDemo(3);GameTestAccess::addBarrel(g,{300,600},3,true);
        g.advance(2);require(g.bullets().size()==5,"2.5 tiros/s durante 2s");
        for(const auto& b:g.bullets()) { near(length(b.velocity),10);require(b.owner==Owner::Enemy,"Dono"); }
        const auto& last=g.bullets().back();near(length(last.position-g.barrels().at(0).enemy.muzzle()),10*Game::timeStep);
    });
    test("Frequencia zero desativa tiros sem dividir por zero", [] {
        auto c=config();c.enemyShotsPerSecond=0;c.barrelSpeed=0;auto g=emptyGame(c);g.toggleDemo(3);
        GameTestAccess::addBarrel(g,{300,600},3,true);g.advance(20);require(g.bullets().empty(),"Frequencia zero");
    });
    test("Destruir barril elimina seu inimigo e disparos futuros", [] {
        auto c=config();c.enemyShotsPerSecond=5;auto g=emptyGame(c);g.toggleDemo(3);
        GameTestAccess::addBarrel(g,{300,600},1,true);bulletAtBarrel(g);g.advance(1);
        require(g.barrels().empty() && g.bullets().empty(),"Inimigo destruido");
    });
    test("Tiros ja disparados persistem apos a morte do oponente", [] {
        auto c=config();c.enemyShotsPerSecond=5;c.enemyShotSpeed=10;auto g=emptyGame(c);g.toggleDemo(3);
        GameTestAccess::addBarrel(g,{300,600},1,true);g.advance(0.2);require(g.bullets().size()==1,"Tiro anterior");
        bulletAtBarrel(g);require(g.barrels().empty() && g.bullets().size()==1,"Tiro independente do atirador");
    });
    test("Pausa congela tudo, retoma sem acumular tempo", [] {
        auto g=emptyGame();g.fire();g.togglePause();const auto b=g.bullets().at(0);const auto p=g.player();
        g.setKey('w',true);g.aim(20);g.fire();g.advance(30);
        near(length(g.player().position-p.position),0);near(length(g.bullets().at(0).position-b.position),0);
        near(g.elapsed(),0);require(g.bullets().size()==1,"Clique pausado");
        g.togglePause();g.advance(0.1);near(g.elapsed(),0.1);near(length(g.player().position-p.position),0);
    });
    test("Ajuda suspende simulacao e respeita pausa anterior", [] {
        auto g=emptyGame();Controller input(g);input.special(1);require(g.helpVisible(),"F1 abre");g.advance(1);near(g.elapsed(),0);
        input.keyDown('h');g.advance(0.1);near(g.elapsed(),0.1);
        g.togglePause();input.special(1);input.special(1);require(g.paused(),"Pausa preservada");
    });
    test("Atalhos F2/F3/F4/F5 permitem demonstrar mecanismos isolados", [] {
        Game g(config());Controller input(g);input.special(2);input.special(3);input.special(4);input.special(5);
        const auto b=g.barrels().at(0);g.advance(5);
        require(g.barrels().size()==1 && g.bullets().empty(),"Spawning e tiros desligados");
        near(g.barrels().at(0).position.y,b.position.y);require(g.demo().hitboxes,"Colisoes visiveis");
    });
    test("Sair da janela limpa teclas e referencia do mouse", [] {
        auto g=emptyGame();Controller input(g);input.keyDown('w');input.motion(20);input.leave();input.motion(400);
        const auto p=g.player();g.advance(0.5);near(length(g.player().position-p.position),0);near(g.player().gunAngle,0);
    });
    test("ESC solicita encerramento", [] { auto g=emptyGame();Controller input(g);require(!input.keyDown(27),"ESC"); });
    test("Tempo: resultados identicos a 20, 30, 60, 144 e 240 FPS", [] {
        std::vector<Game> games;
        for(int fps:{20,30,60,144,240}) {
            auto c=config();c.enemyShotsPerSecond=0;c.spawnInterval=0.7;
            games.emplace_back(c);auto& g=games.back();g.setKey('w',true);g.setKey('d',true);
            for(int i=0;i<fps*3;++i)g.advance(1.0/fps);
        }
        for(const auto& g:games) {
            near(length(g.player().position-games[0].player().position),0);near(g.player().angle,games[0].player().angle);
            near(g.elapsed(),3);require(g.barrels().size()==games[0].barrels().size(),"Nascimento");
            for(std::size_t i=0;i<g.barrels().size();++i)near(length(g.barrels()[i].position-games[0].barrels()[i].position),0);
        }
    });
    test("Tempo irregular e uma chamada longa equivalem a passos fixos", [] {
        auto a=emptyGame(),b=emptyGame();a.setKey('w',true);b.setKey('w',true);a.advance(0.75);
        for(double dt:{0.001,0.019,0.25,0.08,0.4}) b.advance(dt);
        near(length(a.player().position-b.player().position),0);near(a.elapsed(),b.elapsed());
    });
    test("Tempo invalido rejeitado sem alterar estado", [] {
        auto g=emptyGame();for(double dt:{-1.0,noHit,std::numeric_limits<double>::quiet_NaN()}) {
            bool caught=false;try {g.advance(dt);} catch(...) {caught=true;}require(caught,"Tempo invalido");
        }near(g.elapsed(),0);
    });
    test("Semente fixa torna nascimento e reinicio reproduziveis", [] {
        Game a(config()),b(config());for(int i=0;i<50;++i) {GameTestAccess::spawn(a);GameTestAccess::spawn(b);}
        for(std::size_t i=0;i<a.barrels().size();++i) {
            near(a.barrels()[i].position.x,b.barrels()[i].position.x);require(a.barrels()[i].hasEnemy==b.barrels()[i].hasEnemy,"Mesmo sorteio");
        }
        const auto first=a.barrels()[0];a.reset();near(a.barrels()[0].position.x,first.position.x);
    });
    test("Simulacao prolongada remove entidades e mantem estado finito", [] {
        auto c=config();c.playerSpeed=0;c.barrelWidth=20;c.barrelHeight=20;c.barrelSpeed=100;c.enemyShotsPerSecond=0;
        auto g=emptyGame(c);
        for(int i=0;i<1200;++i) {
            if(i%10==0)GameTestAccess::addBarrel(g,{40,700},2);
            if(i%2==0)g.fire();
            g.advance(0.1);
        }
        require(g.barrels().size()<10 && g.bullets().size()<10,"Entidades sem vazamento");
        require(g.status()==Status::Playing && std::isfinite(g.elapsed()),"Estado finito");near(g.elapsed(),120);
    });
    test("Campanha: dez fases, miniboss apenas na 5 e boss final na 10", [] {
        auto c=config();c.barrelsToWin=1;c.barrelHitPoints=1;c.barrelSpeed=0;c.enemyShotsPerSecond=0;
        Game g(c);
        for(int stage=1;stage<=10;++stage) {
            require(g.stage()==stage && g.status()==Status::Playing,"Ordem das fases");
            require(g.isBossStage()==(stage==5 || stage==10),"Fases dos chefes");
            require((g.boss()!=nullptr)==g.isBossStage(),"Chefe exclusivo");
            if(stage==5)require(g.boss()->kind==BarrelKind::Miniboss,"Miniboss");
            if(stage==10)require(g.boss()->kind==BarrelKind::FinalBoss,"Boss final");
            const int hits=g.barrels().at(0).hitPoints;
            for(int i=0;i<hits;++i)bulletAtBarrel(g);
            require(g.stageDestroyed()==1 && g.destroyed()==stage,"Placar total e da fase");
            if(stage<10) {
                require(g.transitioning() && g.status()==Status::Playing,"Intervalo entre fases");
                g.advance(Game::stageBreak);
            } else require(g.status()==Status::Won && !g.transitioning(),"Vitoria somente no final");
        }
        g.advance(100);require(g.stage()==10 && g.destroyed()==10,"Nao existe fase 11");
    });
    test("Transicao limpa perigos, bloqueia comandos e reposiciona jogador", [] {
        auto c=config();c.barrelsToWin=1;auto g=emptyGame(c);
        GameTestAccess::player(g).position={100,200};GameTestAccess::player(g).angle=90;
        GameTestAccess::addBarrel(g,{300,600},1);
        GameTestAccess::addBullet(g,{20,700},{0,-10},Owner::Enemy);
        bulletAtBarrel(g);
        require(g.bullets().empty() && g.barrels().empty(),"Arena limpa");
        const auto p=g.player();g.setKey('w',true);g.aim(50);g.fire();g.advance(1);
        require(g.stage()==1 && g.transitioning() && g.bullets().empty(),"Bloqueio entre fases");
        near(length(g.player().position-p.position),0);near(g.player().gunAngle,p.gunAngle);
        g.advance(1);require(g.stage()==2 && !g.transitioning(),"Passagem automatica");
        near(g.player().position.x,300);near(g.player().position.y,36);near(g.player().angle,0);
        g.advance(0.1);near(g.player().position.y,36);require(g.stageDestroyed()==0 && g.destroyed()==1,"Placar preservado");
    });
    test("Pausa e ajuda congelam intervalo entre fases", [] {
        auto c=config();c.barrelsToWin=1;auto g=emptyGame(c);
        GameTestAccess::addBarrel(g,{300,600},1);bulletAtBarrel(g);
        const double remaining=g.transitionRemaining();g.togglePause();g.advance(10);
        near(g.transitionRemaining(),remaining);g.togglePause();g.toggleHelp();g.advance(10);
        near(g.transitionRemaining(),remaining);g.toggleHelp();g.advance(Game::stageBreak);require(g.stage()==2,"Retomada");
    });
    test("Dificuldade cresce em velocidade, resistencia e frequencia de barris", [] {
        Game g(config());double speed=0,interval=1000;int hp=0;
        for(int stage:{1,2,3,4,6,7,8,9}) {
            GameTestAccess::stage(g,stage);
            require(g.barrelSpeed()>speed && g.spawnInterval()<interval,"Velocidade e intervalo");
            require(g.barrels()[0].hitPoints>=hp,"Resistencia nao diminui");
            speed=g.barrelSpeed();interval=g.spawnInterval();hp=g.barrels()[0].hitPoints;
        }
        require(hp>g.config().barrelHitPoints,"Fases finais mais resistentes");
    });
    test("Chefes tem corpo maior e vidas proprias", [] {
        Game g(config());GameTestAccess::stage(g,5);const auto mini=*g.boss();
        require(mini.width>g.config().barrelWidth && mini.height>g.config().barrelHeight,"Corpo miniboss");
        require(mini.hitPoints==12 && mini.maxHitPoints==12,"Vida miniboss");
        GameTestAccess::stage(g,10);const auto final=*g.boss();
        require(final.width>mini.width && final.height>mini.height && final.hitPoints==30,"Boss final maior");
    });
    test("Colisao de tiro considera toda a largura ampliada do chefe", [] {
        Game g(config());GameTestAccess::stage(g,5);g.toggleDemo(2);g.toggleDemo(3);
        const auto b=*g.boss();
        GameTestAccess::addBullet(g,{b.position.x+b.width/2-1,b.position.y},{0,0});g.advance(Game::timeStep);
        require(g.boss()->hitPoints==b.hitPoints-1,"Extremidade do chefe nao atingivel");
    });
    test("Chefe permanece na metade superior, oscila e nao desaparece", [] {
        auto c=config();c.enemyShotsPerSecond=0;Game g(c);GameTestAccess::stage(g,10);
        const auto initial=*g.boss();double minX=10000,maxX=-10000;
        for(int i=0;i<600;++i) {
            g.advance(0.1);require(g.boss()!=nullptr && g.barrels().size()==1,"Chefe persistente, sem barris extras");
            const auto b=*g.boss();near(b.position.y,initial.position.y);
            require(b.position.x-b.width/2>=0 && b.position.x+b.width/2<=c.arenaWidth,"Limites horizontais");
            require(b.position.y-b.height/2>c.arenaHeight/2.0,"Zona superior");
            minX=std::min(minX,b.position.x);maxX=std::max(maxX,b.position.x);
        }
        require(maxX-minX>100,"Movimento lateral");g.toggleDemo(2);const auto p=g.boss()->position;
        g.advance(1);near(length(g.boss()->position-p),0);
    });
    test("Chefe avisa antes da primeira rajada e respeita F3", [] {
        auto c=config();c.enemyShotsPerSecond=100;c.barrelSpeed=0;Game g(c);GameTestAccess::stage(g,5);
        g.advance(0.5);require(g.bullets().empty() && g.boss()->telegraph==0,"Sem ataque instantaneo");
        g.advance(0.3);require(g.bullets().empty() && g.boss()->telegraph>0,"Aviso antes dos tiros");
        g.advance(0.4);require(g.bullets().size()==3 && g.boss()->attackIndex==1,"Primeira rajada");
        g.toggleDemo(3);g.advance(Game::timeStep);require(g.boss()->telegraph==0,"Aviso desativado");
    });
    test("Leque do miniboss e dirigido ao jogador com espacos para desviar", [] {
        Game g(config());GameTestAccess::stage(g,5);GameTestAccess::bossAttack(g,0);
        require(g.bullets().size()==3,"Tres projeteis");
        const auto middle=g.bullets()[1];const auto target=g.player().position-middle.position;
        near(target.x*middle.velocity.y-target.y*middle.velocity.x,0,1e-6);
        for(const auto& b:g.bullets())require(b.owner==Owner::Enemy && b.bossShot && b.velocity.y<0,"Tiro de chefe");
        require(g.bullets()[0].velocity.x*g.bullets()[2].velocity.x<0,"Abertura do leque");
    });
    test("Anel radial tem abertura real de fuga e projeteis para ambos os lados", [] {
        Game g(config());GameTestAccess::stage(g,10);GameTestAccess::bossAttack(g,1);
        require(g.bullets().size()<18 && g.bullets().size()>=14,"Abertura no anel");
        bool up=false,down=false;std::vector<double> angles;
        for(const auto& b:g.bullets()) {
            up=up||b.velocity.y>0;down=down||b.velocity.y<0;
            double angle=std::atan2(b.velocity.y,b.velocity.x)*180/pi;
            if(angle<0)angle+=360;
            angles.push_back(angle);
        }
        require(up&&down,"Distribuicao radial");std::sort(angles.begin(),angles.end());
        double gap=angles.front()+360-angles.back();
        for(std::size_t i=1;i<angles.size();++i)gap=std::max(gap,angles[i]-angles[i-1]);
        require(gap>=59,"Abertura minima para desviar");
    });
    test("Boss final alterna leque, anel e duas armas", [] {
        Game g(config());GameTestAccess::stage(g,10);
        for(int pattern=0;pattern<3;++pattern) {
            GameTestAccess::bullets(g).clear();GameTestAccess::bossAttack(g,pattern);
            if(pattern==0)require(g.bullets().size()==5,"Leque final");
            if(pattern==1)require(g.bullets().size()>10,"Anel final");
            if(pattern==2) {
                require(g.bullets().size()==8,"Rajada dupla");
                require(g.bullets().front().position.x<g.boss()->position.x &&
                        g.bullets().back().position.x>g.boss()->position.x,"Duas origens de tiro");
            }
        }
    });
    test("Boss final entra em furia a meia vida com mais tiros e maior velocidade", [] {
        Game g(config());GameTestAccess::stage(g,10);GameTestAccess::bossAttack(g,0);
        const double before=length(g.bullets().front().velocity);require(!g.boss()->enraged(),"Antes da furia");
        GameTestAccess::barrels(g)[0].hitPoints=g.boss()->maxHitPoints/2;
        GameTestAccess::bullets(g).clear();GameTestAccess::bossAttack(g,0);
        require(g.boss()->enraged() && g.bullets().size()==7,"Furia ativada");
        require(length(g.bullets().front().velocity)>before,"Tiros mais rapidos");
    });
    test("Furia aumenta ritmo de ataque sem perder aviso visual", [] {
        auto c=config();c.barrelSpeed=0;c.enemyShotSpeed=1;
        Game normal(c),enraged(c);GameTestAccess::stage(normal,10);GameTestAccess::stage(enraged,10);
        GameTestAccess::barrels(enraged)[0].hitPoints=enraged.boss()->maxHitPoints/2;
        normal.advance(10);enraged.advance(10);
        require(enraged.boss()->attackIndex>normal.boss()->attackIndex,"Ritmo da furia");
        require(enraged.status()==Status::Playing,"Simulacao de ataques");
    });
    test("Pausa mantem aviso e movimento do chefe congelados", [] {
        auto c=config();c.enemyShotsPerSecond=100;Game g(c);GameTestAccess::stage(g,10);g.advance(0.8);
        const auto b=*g.boss();g.togglePause();g.advance(5);
        near(g.boss()->telegraph,b.telegraph);near(length(g.boss()->position-b.position),0);
        require(g.bullets().empty(),"Sem tiros durante pausa");
    });
    test("Reiniciar na fase final retorna a primeira e remove a furia", [] {
        Game g(config());GameTestAccess::stage(g,10);GameTestAccess::barrels(g)[0].hitPoints=1;
        GameTestAccess::bossAttack(g,2);g.reset();
        require(g.stage()==1 && !g.boss() && !g.transitioning() && g.destroyed()==0 && g.bullets().empty(),"Campanha reiniciada");
    });
    test("Chefes sao deterministas em taxas de quadros diferentes", [] {
        auto c=config();c.enemyShotSpeed=1;Game a(c),b(c);GameTestAccess::stage(a,10);GameTestAccess::stage(b,10);
        for(int i=0;i<20*4;++i)a.advance(1.0/20);
        for(int i=0;i<144*4;++i)b.advance(1.0/144);
        near(length(a.boss()->position-b.boss()->position),0);
        require(a.boss()->attackIndex==b.boss()->attackIndex && a.bullets().size()==b.bullets().size(),"Mesmo ataque");
        for(std::size_t i=0;i<a.bullets().size();++i)near(length(a.bullets()[i].position-b.bullets()[i].position),0);
    });
    std::cout << '\n' << passed << " testes passaram; " << failed << " falharam.\n";
    return failed ? 1 : 0;
}
