#include "renderer.hpp"
#include "support.hpp"
#include <GL/freeglut.h>
#include <array>
#include <chrono>
#include <filesystem>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>

using namespace cg;
namespace {
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
using Pixel = std::array<unsigned char,3>;
std::vector<unsigned char> frame(int w, int h) {
    std::vector<unsigned char> data(static_cast<std::size_t>(w)*h*3);
    glReadBuffer(GL_BACK);glPixelStorei(GL_PACK_ALIGNMENT,1);
    glReadPixels(0,0,w,h,GL_RGB,GL_UNSIGNED_BYTE,data.data());return data;
}
Pixel pixel(const std::vector<unsigned char>& data, int width, int x, int y) {
    const auto i=static_cast<std::size_t>(y*width+x)*3;
    return {data.at(i),data.at(i+1),data.at(i+2)};
}
int differences(const std::vector<unsigned char>& a,const std::vector<unsigned char>& b,int width,int x,int y,int w,int h) {
    int count=0;
    for(int row=y;row<y+h;++row) for(int col=x;col<x+w;++col)
        count+=pixel(a,width,col,row)!=pixel(b,width,col,row);
    return count;
}
bool isGreen(Pixel p) { return p[1]>170 && p[0]<150 && p[2]<140; }
bool isPurple(Pixel p) { return p[0]>160 && p[1]<140 && p[2]>190; }
bool isRed(Pixel p) { return p[0]>190 && p[1]<150 && p[2]<150; }
void emptyDisplay() {}
void resizeWindow(int width,int height) {
    glutReshapeWindow(width,height);
    for(int i=0;i<20;++i) {
        glutMainLoopEvent();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}
}

int main(int argc,char** argv) {
    glutInit(&argc,argv);glutInitContextVersion(1,0);glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB);
    glutInitWindowSize(600,800);const int window=glutCreateWindow("Testes graficos CG");
    glutDisplayFunc(emptyDisplay);
    Renderer renderer;renderer.resize(600,800);
    std::filesystem::create_directories("build/evidencias");
    Config config;config.fixedSeed=true;config.seed=42;config.barrelSpeed=0;
    Game game(config);GameTestAccess::empty(game);
    GameTestAccess::player(game).position={300,150};
    GameTestAccess::addBarrel(game,{150,550},3,false);
    GameTestAccess::addBarrel(game,{440,650},5,true);
    int passed=0,failed=0;
    auto test=[&](const char* name,const std::function<void()>& body) {
        try {body();require(glGetError()==GL_NO_ERROR,"Erro OpenGL");++passed;std::cout<<"OK  "<<name<<'\n';}
        catch(const std::exception& e) {++failed;std::cerr<<"FALHA  "<<name<<": "<<e.what()<<'\n';}
    };
    std::cout<<"OpenGL: "<<glGetString(GL_VERSION)<<"\nRenderer: "<<glGetString(GL_RENDERER)<<'\n';
    renderer.draw(game);const auto initial=frame(600,800);renderer.saveFrame("build/evidencias/01-arena.ppm");
    test("Contexto real com double buffer e janela 600x800",[&] {
        GLboolean doubleBuffer=GL_FALSE;glGetBooleanv(GL_DOUBLEBUFFER,&doubleBuffer);require(doubleBuffer,"Double buffer ausente");
        require(glutGet(GLUT_WINDOW_WIDTH)==600 && glutGet(GLUT_WINDOW_HEIGHT)==800,"Dimensoes incorretas");
        GLint viewport[4];glGetIntegerv(GL_VIEWPORT,viewport);require(viewport[0]==0 && viewport[1]==0 && viewport[2]==600 && viewport[3]==800,"Viewport");
    });
    test("Arena preenche a janela sem margens",[&] {
        for(const auto p:{pixel(initial,600,0,0),pixel(initial,600,599,0),pixel(initial,600,0,799)})
            require(p[2]>50 && p[2]>p[1] && p[1]>p[0],"Canto nao pertence a arena");
    });
    test("Cabecas verde e roxa sao desenhadas nas posicoes do modelo",[&] {
        require(isGreen(pixel(initial,600,300,150)),"Jogador verde");
        require(isPurple(pixel(initial,600,440,650)),"Inimigo roxo");
        require(!isGreen(pixel(initial,600,283,167)),"Cabeça deve ser circular");
    });
    test("Duas pernas pretas e arma vermelha proporcional ao raio",[&] {
        for(int x:{289,311}) {const auto p=pixel(initial,600,x,127);require(p[0]<10 && p[1]<10 && p[2]<15,"Pernas");}
        require(isRed(pixel(initial,600,319,170)),"Arma vermelha");
    });
    test("Barril retangular laranja com dimensoes do XML",[&] {
        const auto inside=pixel(initial,600,115,522),outside=pixel(initial,600,110,520);
        require(inside[0]>150 && inside[1]>60 && inside[2]<60,"Barril laranja");
        require(outside[2]>outside[0],"Exterior do retangulo");
    });
    test("Linha central tracejada",[&] {
        require(pixel(initial,600,4,399)!=pixel(initial,600,14,399) || pixel(initial,600,4,400)!=pixel(initial,600,14,400),"Tracejado ausente");
    });
    test("Fundo uniforme sem quadriculado",[&] {
        const auto background=pixel(initial,600,39,239);
        for(int x:{39,40,41,79,80,81,119,120,121})
            for(int y:{239,240,241,279,280,281})
                require(pixel(initial,600,x,y)==background,"Linha do antigo quadriculado ainda visivel");
    });
    test("Animacao das pernas muda pixels, rotacao mantem partes unidas",[&] {
        GameTestAccess::player(game).walkPhase=pi/2;renderer.draw(game);const auto moved=frame(600,800);
        require(differences(initial,moved,600,270,115,60,65)>50,"Passada invisivel");
        GameTestAccess::player(game).angle=90;renderer.draw(game);const auto rotated=frame(600,800);
        require(isGreen(pixel(rotated,600,300,150)),"Cabeca deslocada ao girar");
        require(isRed(pixel(rotated,600,280,169)),"Arma nao acompanhou o corpo");
        renderer.saveFrame("build/evidencias/02-corpo-girado.ppm");
        GameTestAccess::player(game).angle=0;GameTestAccess::player(game).walkPhase=0;
    });
    test("Mira em -30 e +30 graus produz imagens distintas",[&] {
        GameTestAccess::player(game).gunAngle=-30;renderer.draw(game);auto left=frame(600,800);
        GameTestAccess::player(game).gunAngle=30;renderer.draw(game);auto right=frame(600,800);
        require(differences(left,right,600,270,115,90,105)>150,"Arma nao girou");
        renderer.saveFrame("build/evidencias/03-mira.ppm");GameTestAccess::player(game).gunAngle=0;
    });
    test("Vida do barril e placar sao atualizados visualmente",[&] {
        renderer.draw(game);const auto before=frame(600,800);
        GameTestAccess::addBullet(game,{150,550},{0,0});game.advance(Game::timeStep);renderer.draw(game);
        const auto damaged=frame(600,800);require(differences(before,damaged,600,135,537,30,26)>10,"Numero da resistencia nao mudou");
        for(int i=0;i<2;++i) {GameTestAccess::addBullet(game,{150,550},{0,0});game.advance(Game::timeStep);}
        renderer.draw(game);const auto destroyed=frame(600,800);
        require(differences(damaged,destroyed,600,310,760,285,35)>10,"Placar nao mudou");
        require(differences(damaged,destroyed,600,112,518,76,64)>1000,"Barril nao desapareceu");
        renderer.saveFrame("build/evidencias/04-placar.ppm");
    });
    test("Tiros dos dois lados sao visiveis com cores diferentes",[&] {
        GameTestAccess::addBullet(game,{100,200},{0,0},Owner::Player,4);
        GameTestAccess::addBullet(game,{120,200},{0,0},Owner::Enemy,4);renderer.draw(game);auto f=frame(600,800);
        const auto a=pixel(f,600,100,200),b=pixel(f,600,120,200);
        require(a[0]>200 && a[1]>200 && b[0]>200 && b[1]<150 && b[2]>150,"Cor dos tiros");
    });
    test("Inimigo caminha visualmente mesmo com barril parado",[&] {
        renderer.draw(game);const auto a=frame(600,800);game.advance(0.1);renderer.draw(game);const auto b=frame(600,800);
        require(differences(a,b,600,420,635,40,50)>20,"Pernas inimigas paradas");
    });
    test("Ajuda, pausa, vitoria e derrota sao telas distintas",[&] {
        renderer.draw(game);auto prior=frame(600,800);
        game.toggleHelp();renderer.draw(game);auto help=frame(600,800);
        require(differences(prior,help,600,100,280,400,240)>20000,"Ajuda invisivel");
        renderer.saveFrame("build/evidencias/05-ajuda.ppm");game.toggleHelp();game.togglePause();renderer.draw(game);
        auto pause=frame(600,800);renderer.saveFrame("build/evidencias/06-pausa.ppm");
        game.togglePause();GameTestAccess::terminal(game,Status::Won);renderer.draw(game);auto won=frame(600,800);
        require(differences(pause,won,600,80,340,440,170)>200,"Vitoria igual a pausa");
        renderer.saveFrame("build/evidencias/07-vitoria.ppm");
        GameTestAccess::terminal(game,Status::Lost,LossReason::Shot);renderer.draw(game);auto lost=frame(600,800);
        require(differences(won,lost,600,80,340,440,170)>200,"Derrota igual a vitoria");
        renderer.saveFrame("build/evidencias/08-derrota-tiro.ppm");
        GameTestAccess::terminal(game,Status::Lost,LossReason::Barrel);renderer.draw(game);
        require(differences(lost,frame(600,800),600,80,340,440,170)>100,"Motivo da derrota nao mudou");
        renderer.saveFrame("build/evidencias/09-derrota-barril.ppm");
    });
    test("Pilha de matrizes balanceada apos 100 quadros",[&] {
        for(int i=0;i<100;++i)renderer.draw(game);
        GLint depth=0;glGetIntegerv(GL_MODELVIEW_STACK_DEPTH,&depth);require(depth==1,"Vazamento de push/pop");
    });
    test("HUD mostra fase atual e progresso independente do placar total",[&] {
        Game campaign(config);renderer.draw(campaign);const auto first=frame(600,800);
        GameTestAccess::stage(campaign,7);renderer.draw(campaign);const auto seventh=frame(600,800);
        require(differences(first,seventh,600,0,775,170,25)>5,"Numero da fase nao mudou");
        renderer.saveFrame("build/evidencias/13-fase-7.ppm");
    });
    test("Conclusao de fase exibe intervalo antes da proxima",[&] {
        Config c=config;c.barrelsToWin=1;Game campaign(c);GameTestAccess::empty(campaign);
        GameTestAccess::addBarrel(campaign,{300,550},1);renderer.draw(campaign);const auto before=frame(600,800);
        GameTestAccess::addBullet(campaign,{300,550},{0,0});campaign.advance(Game::timeStep);renderer.draw(campaign);
        require(campaign.transitioning(),"Transicao nao ocorreu");
        require(differences(before,frame(600,800),600,70,310,460,180)>20000,"Mensagem de fase invisivel");
        renderer.saveFrame("build/evidencias/14-fase-concluida.ppm");
    });
    test("Miniboss tem corpo proprio e barra de vida que diminui",[&] {
        Game campaign(config);GameTestAccess::stage(campaign,5);renderer.draw(campaign);const auto full=frame(600,800);
        const auto b=*campaign.boss();
        require(b.width>config.barrelWidth,"Escala miniboss");
        GameTestAccess::barrels(campaign)[0].hitPoints-=3;renderer.draw(campaign);const auto damaged=frame(600,800);
        require(differences(full,damaged,600,0,710,600,40)>300,"Barra de vida nao mudou");
        renderer.saveFrame("build/evidencias/15-miniboss.ppm");
    });
    test("Boss final tem visual distinto e sinaliza furia",[&] {
        Game campaign(config);GameTestAccess::stage(campaign,5);renderer.draw(campaign);const auto mini=frame(600,800);
        GameTestAccess::stage(campaign,10);renderer.draw(campaign);const auto final=frame(600,800);
        require(differences(mini,final,600,150,535,300,150)>6000,"Bosses iguais");
        renderer.saveFrame("build/evidencias/16-boss-final.ppm");
        GameTestAccess::barrels(campaign)[0].hitPoints=campaign.boss()->maxHitPoints/2;
        renderer.draw(campaign);const auto furious=frame(600,800);
        require(differences(final,furious,600,270,600,60,40)>10,"Olhos da furia invisiveis");
        require(differences(final,furious,600,0,710,600,40)>300,"Vida/furia no HUD");
        renderer.saveFrame("build/evidencias/17-furia.ppm");
    });
    test("Aviso de ataque e projeteis radiais sao visiveis",[&] {
        auto c=config;c.enemyShotsPerSecond=100;Game campaign(c);GameTestAccess::stage(campaign,10);
        renderer.draw(campaign);const auto before=frame(600,800);campaign.advance(0.8);renderer.draw(campaign);
        const auto warning=frame(600,800);
        require(campaign.boss()->telegraph>0,"Aviso ausente");
        require(differences(before,warning,600,150,525,300,170)>100,"Carregamento das armas invisivel");
        renderer.saveFrame("build/evidencias/18-aviso.ppm");
        GameTestAccess::bossAttack(campaign,1);campaign.toggleDemo(3);campaign.advance(0.8);renderer.draw(campaign);
        require(differences(warning,frame(600,800),600,0,410,600,300)>200,"Anel de tiros invisivel");
        renderer.saveFrame("build/evidencias/19-anel.ppm");
    });
    test("Dimensoes alternativas, resize e configuracao estreita",[&] {
        Config small=config;small.arenaWidth=200;small.arenaHeight=500;small.playerRadius=12;
        Game alternate(small);GameTestAccess::empty(alternate);GameTestAccess::addBarrel(alternate,{100,340},6,true);
        resizeWindow(200,500);renderer.resize(200,500);renderer.draw(alternate);
        renderer.saveFrame("build/evidencias/10-arena-200x500.ppm");
        const auto data=frame(200,500);require(isGreen(pixel(data,200,100,24)),"Projecao alternativa");
        alternate.toggleHelp();renderer.draw(alternate);renderer.saveFrame("build/evidencias/11-ajuda-200x500.ppm");
        resizeWindow(420,600);renderer.resize(420,600);alternate.toggleHelp();renderer.draw(alternate);
        require(glutGet(GLUT_WINDOW_WIDTH)==420,"Resize real");renderer.saveFrame("build/evidencias/12-resize.ppm");
    });
    glutDestroyWindow(window);
    std::cout<<'\n'<<passed<<" testes graficos passaram; "<<failed<<" falharam.\n";
    return failed?1:0;
}
