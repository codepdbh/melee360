"""Extract original stage music selection, character themes and L/R input."""
import argparse
from pathlib import Path
from generate_menu_sources import functions
from generate_campaign_sources import declaration

ROOT = Path(__file__).resolve().parents[1] / 'upstream/melee-pc/src/melee'


def generate(output):
    ground = (ROOT / 'gr/ground.c').read_text(encoding='utf-8')
    if '#define RANDI_MAX (100)' not in ground:
        raise ValueError('Original music probability range changed')
    audio = (ROOT / 'lb/lbaudio_ax.c').read_text(encoding='utf-8')
    tables = (ROOT / 'lb/lbaudio_ax.static.h').read_text(encoding='utf-8')
    vs = (ROOT / 'gm/gmvs.c').read_text(encoding='utf-8')
    sentinel = 'static const int BGM_Undefined = -1;'
    if sentinel not in ground:
        raise ValueError('Original undefined music sentinel changed')
    text = '/* Generated original game code; do not edit. */\n#define RANDI_MAX (100)\n' + sentinel + '\n'
    text += declaration(tables, 'static u8 unk_arr_803BC4A0') + '\n'
    text += 'static ' + functions(audio)['lbAudioAx_8002305C'][1] + '\n'
    text += functions(ground)['Ground_801C24F8'][1] + '\n'
    text += 'static ' + functions(vs)['fn_8016E5C0'][1] + '\n'
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(text, encoding='utf-8')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    generate(parser.parse_args().output)
