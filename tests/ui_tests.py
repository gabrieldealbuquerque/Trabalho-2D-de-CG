#!/usr/bin/env python3
"""Integração da aplicação nativa: eventos X11 reais, callbacks GLUT e framebuffer."""
import json
import math
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / 'build' / 'evidencias' / 'ui'
OUTPUT.mkdir(parents=True, exist_ok=True)
if not shutil.which('xdotool') or not os.environ.get('DISPLAY'):
    raise SystemExit('Este teste requer xdotool e uma tela X11 (ex.: xvfb-run -a make test-ui).')
passed = 0


def check(condition, name):
    global passed
    if not condition:
        raise AssertionError(name)
    passed += 1
    print('OK ', name, flush=True)


def xml(*, speed=0, hits=2, target=1, enemy_rate=0, seed=42, width=600, player_radius=18):
    return f'''<jogo><arena largura="{width}" altura="800"/>
<jogador raioCabeca="{player_radius}" velocidade="180" velocidadeGiro="150" velocidadeTiro="2400"/>
<inimigo raioCabeca="15" tirosPorSegundo="{enemy_rate}" velocidadeTiro="600"/>
<barril largura="{width}" altura="32" numeroTiros="{hits}" nParaGanhar="{target}" velocidade="{speed}"/>
<outros intervaloBarris="3600" sensibilidadeMouse="0.25" semente="{seed}"/></jogo>'''


class App:
    def __init__(self, name, config):
        self.directory = OUTPUT / name
        self.directory.mkdir(exist_ok=True)
        for old in self.directory.glob('quadro-*'):
            old.unlink()
        self.config = self.directory / 'configuracoes.xml'
        self.config.write_text(config)
        self.log = (self.directory / 'execucao.log').open('w')
        self.process = subprocess.Popen(
            [str(ROOT / 'trabalhocg'), '--config', str(self.config), '--diagnostico', str(self.directory)],
            cwd=tempfile.gettempdir(), stdout=self.log, stderr=subprocess.STDOUT)
        self.counter = 0
        deadline = time.monotonic() + 8
        try:
            while time.monotonic() < deadline:
                result = subprocess.run(['xdotool', 'search', '--pid', str(self.process.pid), '--name', 'Ataque de Barris'],
                                        text=True, capture_output=True)
                if result.returncode == 0 and result.stdout.strip():
                    self.window = result.stdout.strip().splitlines()[0]
                    self.x('windowfocus', '--sync', self.window)
                    self.x('mousemove', '--window', self.window, '300', '350')
                    time.sleep(0.12)
                    return
                if self.process.poll() is not None:
                    raise RuntimeError((self.directory / 'execucao.log').read_text())
                time.sleep(0.05)
            raise RuntimeError('Janela GLUT não abriu')
        except BaseException:
            self.close()
            raise

    def x(self, *args):
        subprocess.run(['xdotool', *map(str, args)], check=True, capture_output=True)

    def key(self, key):
        self.x('key', '--clearmodifiers', key)
        time.sleep(0.035)

    def hold(self, keys, duration):
        for key in keys:
            self.x('keydown', key)
        time.sleep(duration)
        for key in keys:
            self.x('keyup', key)
        time.sleep(0.035)

    def snapshot(self):
        self.counter += 1
        self.key('F12')
        path = self.directory / f'quadro-{self.counter}.json'
        deadline = time.monotonic() + 3
        while time.monotonic() < deadline:
            try:
                return json.loads(path.read_text())
            except (FileNotFoundError, json.JSONDecodeError):
                time.sleep(0.01)
        raise RuntimeError('F12 não gravou estado/framebuffer')

    def next_stage(self, expected):
        deadline = time.monotonic() + 4
        while time.monotonic() < deadline:
            state = self.snapshot()
            if state['stage'] == expected and not state['transitioning']:
                return state
            time.sleep(0.15)
        raise AssertionError(f'Não avançou para a fase {expected}')

    def close(self):
        if self.process.poll() is None:
            self.process.terminate()
            self.process.wait(timeout=5)
        self.log.close()

    def __enter__(self):
        return self

    def __exit__(self, *_):
        self.close()


with App('controles', xml()) as app:
    start = app.snapshot()
    check((start['width'], start['height']) == (600, 800), 'Janela real usa dimensões do XML')
    app.hold(['w'], 0.35)
    moved = app.snapshot()
    check(50 < moved['y'] - start['y'] < 85, 'KeyDown W movimenta jogador com tempo real')
    time.sleep(0.15)
    released = app.snapshot()
    check(abs(released['y'] - moved['y']) < 1, 'KeyUp W interrompe movimento')
    app.hold(['a'], 0.25)
    turned = app.snapshot()
    check(30 < turned['angle'] < 60 and abs(turned['y'] - released['y']) < 1, 'A gira corpo anti-horário sem andar')
    app.hold(['d'], 0.4)
    clockwise = app.snapshot()
    check(clockwise['angle'] < 0, 'D gira corpo no sentido horário')
    app.hold(['s'], 0.15)
    backward = app.snapshot()
    check(backward['y'] < clockwise['y'], 'S anda para trás')
    app.hold(['w', 'a'], 0.5)
    arc = app.snapshot()
    check(abs(arc['x']-backward['x'])>1 and arc['angle']>backward['angle'], 'W+A simultâneos descrevem arco')
    app.x('mousemove', '--window', app.window, 250, 350)
    left = app.snapshot()
    app.x('mousemove', '--window', app.window, 450, 350)
    right = app.snapshot()
    check(left['gunAngle'] > 0 and right['gunAngle'] == -30, 'Movimento real do mouse controla arma e limite')
    app.x('mousemove', '--window', app.window, 10, 350)
    check(app.snapshot()['gunAngle'] == 30, 'Arma atinge limite esquerdo de +30 graus')
    app.key('p')
    paused = app.snapshot()
    app.hold(['w'], 0.15)
    still = app.snapshot()
    check(still['paused'] and still['elapsed']==paused['elapsed'] and still['y']==paused['y'], 'P congela simulação')
    app.key('p')
    app.key('F1')
    help_state = app.snapshot()
    check(help_state['help'] and help_state['status']==0, 'F1 abre ajuda')
    app.key('h')
    check(not app.snapshot()['help'], 'H fecha ajuda')
    for key in ['F2','F3','F4','F5']:
        app.key(key)
    demo=app.snapshot()
    check(not demo['barrelMotion'] and not demo['enemyFire'] and not demo['spawning'] and demo['hitboxes'], 'F2 a F5 mudam recursos de demonstração')
    app.key('r')
    restarted=app.snapshot()
    check(restarted['x']==300 and restarted['y']==36 and restarted['destroyed']==0 and restarted['barrelMotion'], 'R reinicia posições, placar e opções')
    app.hold(['w'],2.1)
    check(abs(app.snapshot()['y']-382)<1, 'Cabeça não ultrapassa linha central')
    app.hold(['s'],2.2)
    check(abs(app.snapshot()['y']-18)<1, 'Cabeça não ultrapassa base')
    app.x('windowsize',app.window,420,600)
    time.sleep(0.12)
    resized=app.snapshot()
    check((resized['width'],resized['height'])==(420,600), 'Resize real atualiza viewport')
    app.key('Escape')
    check(app.process.wait(timeout=5)==0, 'ESC encerra aplicação sem erro')

with App('campanha',xml(hits=1)) as app:
    for stage in range(1, 11):
        initial = app.snapshot()
        check(initial['stage']==stage and initial['status']==0, f'Fase {stage}/10 acessível na sequência')
        barrel = initial['barrelStates'][0]
        expected_kind = 1 if stage==5 else 2 if stage==10 else 0
        check(barrel['kind']==expected_kind, f'Fase {stage}: tipo de inimigo correto')
        for hit in range(barrel['hp']):
            app.x('click',1)
            time.sleep(0.38)
            if stage==10 and hit+1==barrel['hp']//2:
                rage=app.snapshot()
                check(rage['barrelStates'][0]['enraged']==1, 'Boss final entra em fúria durante partida real')
        cleared = app.snapshot()
        check(cleared['destroyed']==stage and cleared['barrels']==0, f'Fase {stage}: inimigo derrotado e arena limpa')
        if stage<10:
            check(cleared['transitioning'] and cleared['status']==0, f'Fase {stage}: avanço sem vitória antecipada')
            app.next_stage(stage+1)
        else:
            check(cleared['status']==1 and not cleared['transitioning'], 'Vitória real somente após boss da fase 10')
            app.hold(['w'],0.1)
            app.x('click',1)
            terminal=app.snapshot()
            check(terminal['y']==cleared['y'] and terminal['bullets']==0, 'Vitória bloqueia movimento e tiros')
    app.key('r')
    restarted=app.snapshot()
    check(restarted['status']==0 and restarted['stage']==1 and restarted['destroyed']==0, 'Reinício da campanha após vitória')

with App('transicao',xml()) as app:
    initial=app.snapshot()
    check(initial['barrelStates'][0]['hp']==2, 'Barril começa com resistência configurada')
    app.x('mousedown',1)
    time.sleep(0.55)
    one=app.snapshot()
    check(one['barrelStates'][0]['hp']==1 and one['destroyed']==0, 'Primeiro clique tira uma vida sem pontuar')
    time.sleep(0.25)
    held=app.snapshot()
    check(held['barrelStates'][0]['hp']==1 and held['bullets']==0, 'Segurar botão não dispara novamente')
    app.x('mouseup',1)
    app.x('click',1)
    time.sleep(0.45)
    cleared=app.snapshot()
    check(cleared['status']==0 and cleared['transitioning'] and cleared['destroyed']==1 and cleared['barrels']==0,
          'Segundo clique conclui primeira fase sem encerrar jogo')
    app.hold(['w'],0.1)
    app.x('click',1)
    terminal=app.snapshot()
    check(terminal['y']==cleared['y'] and terminal['bullets']==0, 'Intervalo entre fases bloqueia movimento e tiros')
    app.key('r')
    check(app.snapshot()['stage']==1, 'Reinício durante intervalo entre fases')

with App('derrota-barril',xml(speed=700)) as app:
    time.sleep(1.2)
    lost=app.snapshot()
    check(lost['status']==2 and lost['lossReason']==1, 'Contato real barril/cabeça resulta em derrota')
    app.key('r')
    restarted=app.snapshot()
    check(restarted['status']==0 and restarted['destroyed']==0, 'Reinício após derrota')

with App('derrota-tiro',xml(enemy_rate=4,seed=2)) as app:
    first=app.snapshot()
    check(first['barrelStates'][0]['enemy']==1, 'Cenário real com inimigo sobre barril')
    time.sleep(1.6)
    lost=app.snapshot()
    check(lost['status']==2 and lost['lossReason']==2, 'Disparo real do inimigo atinge jogador e encerra jogo')

result=subprocess.run([str(ROOT/'trabalhocg'),'--verificar-config'],cwd=tempfile.gettempdir(),capture_output=True,text=True)
check(result.returncode==0 and '600x800' in result.stdout, 'XML é encontrado junto ao executável ao iniciar em outra pasta')
result=subprocess.run([str(ROOT/'trabalhocg'),'--config',str(ROOT/'docs'/'configuracoes-original.xml'),'--verificar-config'],capture_output=True,text=True)
check(result.returncode!=0 and 'invalido' in result.stderr, 'Modelo XML original com reticências produz erro claro')
print(f'\n{passed} verificações de interface passaram. Evidências: {OUTPUT}')
