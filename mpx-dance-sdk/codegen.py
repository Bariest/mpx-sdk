#!/usr/bin/env python3
"""
mpx-dance-codegen — MPX-Dance Choreography to WASM C Source Generator
"""
import argparse, hashlib, json, math, os, random, sys
from pathlib import Path

MOVE_IDS = {
    "none":0,"stop":1,"hold":2,"look_up":3,"look_down":4,"look_left":5,"look_right":6,
    "look_upperleft":7,"look_upperright":8,"look_lowerleft":9,"look_lowerright":10,
    "nod":11,"headbang":12,"head_ellipse":13,"seek":14,
    "shoulder_shrug":16,"right_shoulder_shrug":17,"left_shoulder_shrug":18,
    "butt_shrug":22,"right_butt_shrug":23,"left_butt_shrug":24,
    "backleg_lift":25,"front_kick":26,
    "lean_left":27,"lean_right":28,"body_row":29,"swagger":30,
    "wiggle":31,"left_wiggle":32,"right_wiggle":33,"body_ellipse":34,
    "raise_body":35,"lower_body":36,"squat":37,"bounce":38,
    "greet":39,"stretch":40,"roll_body":41,"pitch_body":42,
    "dip":64,"spin":65,
    "twerk":128,"advance":129,"back":130,"left":131,"right":132,
    "turn_left":133,"turn_right":134,"jump":135,"step":136,"init":137,
    "lean":27,"lean-right":28,"rotate_cw":134,"rotate_ccw":133,
    "raise-body":35,"lower-body":36,
}

MOVE_ALIASES = {
    "look_upper_right":"look_upperright","look_lower_left":"look_lowerleft",
    "look_lower_right":"look_lowerright","lean-right":"lean_right",
    "lean-left":"lean_left","lean":"lean_left","look-up":"look_up",
    "look-down":"look_down","look-left":"look_left","look-right":"look_right",
    "raise-body":"raise_body","lower-body":"lower_body",
    "rotate_cw":"turn_right","rotate_ccw":"turn_left",
}

GENRE_ALIASES = {
    "hip-hop":"hiphop","hiphop":"hiphop","edm":"electronic","techno":"electronic",
    "metal":"rock","punk":"rock","alternative":"rock","k-pop":"pop","j-pop":"pop",
    "blues":"jazz","r&b":"jazz","soul":"jazz",
    "indie":"folk","acoustic":"folk",
}

ANGLE_TYPES = {
    "look_up":{"type":"pitch","default":20},"look_down":{"type":"pitch","default":20},
    "look_left":{"type":"yaw","default":30},"look_right":{"type":"yaw","default":30},
    "look_upperleft":{"type":"pitch","default":20},"look_upperright":{"type":"pitch","default":20},
    "look_lowerleft":{"type":"pitch","default":20},"look_lowerright":{"type":"pitch","default":20},
    "lean_left":{"type":"roll","default":10},"lean_right":{"type":"roll","default":10},
    "body_row":{"type":"roll","default":10},"swagger":{"type":"roll","default":10},
    "wiggle":{"type":"roll","default":5},"left_wiggle":{"type":"roll","default":5},"right_wiggle":{"type":"roll","default":5},
    "raise_body":{"type":"height","default":10},"lower_body":{"type":"height","default":10},
    "squat":{"type":"height","default":20},"bounce":{"type":"height","default":3},
    "turn_left":{"type":"duration","default":500},"turn_right":{"type":"duration","default":500},
    "advance":{"type":"duration","default":500},"back":{"type":"duration","default":500},
    "left":{"type":"duration","default":500},"right":{"type":"duration","default":500},
    "twerk":{"type":"duration","default":1000},"step":{"type":"duration","default":1000},
    "nod":{"type":"intensity","default":15},"headbang":{"type":"intensity","default":4},
    "head_ellipse":{"type":"intensity","default":10},"body_ellipse":{"type":"intensity","default":10},
    "seek":{"type":"intensity","default":20},"shoulder_shrug":{"type":"intensity","default":10},
    "right_shoulder_shrug":{"type":"intensity","default":10},"left_shoulder_shrug":{"type":"intensity","default":10},
    "butt_shrug":{"type":"intensity","default":10},"right_butt_shrug":{"type":"intensity","default":10},
    "left_butt_shrug":{"type":"intensity","default":10},"backleg_lift":{"type":"intensity","default":15},
    "front_kick":{"type":"intensity","default":20},
    "roll_body":{"type":"duration","default":2000},"pitch_body":{"type":"duration","default":2000},
    "stretch":{"type":"duration","default":0},"greet":{"type":"intensity","default":0},
    "hold":{"type":"duration","default":500},"init":{"type":"duration","default":0},"jump":{"type":"duration","default":0},
}

GENRE_POOLS = {
    "rock":{"moves":["headbang","bounce","dip"],"weights":[0.35,0.35,0.3]},
    "classical":{"moves":["greet","squat","body_ellipse","lean_left"],"weights":[0.2,0.3,0.2,0.3]},
    "pop":{"moves":["lean_left","left_wiggle","shoulder_shrug","nod"],"weights":[0.3,0.3,0.2,0.2]},
    "hiphop":{"moves":["front_kick","backleg_lift","nod"],"weights":[0.3,0.3,0.4]},
    "disco":{"moves":["shoulder_shrug","head_ellipse","look_right","look_upperright"],"weights":[0.35,0.15,0.25,0.25]},
    "electronic":{"moves":["seek","look_up","right_wiggle"],"weights":[0.45,0.35,0.2]},
    "jazz":{"moves":["look_upperleft","right_shoulder_shrug","look_down"],"weights":[0.35,0.35,0.3]},
    "latin":{"moves":["butt_shrug","body_row","left_wiggle"],"weights":[0.4,0.35,0.25]},
    "reggae":{"moves":["left_butt_shrug","look_lowerleft","raise_body","right_butt_shrug"],"weights":[0.25,0.25,0.25,0.25]},
    "folk":{"moves":["look_lowerright","left_shoulder_shrug","look_left"],"weights":[0.3,0.35,0.35]},
}

COMPOUND_EXPANSIONS = {
    "dip":[("lower_body",0),("look_down",15),("lean_left",15),("hold",0),("look_up",0),("lean_right",0),("raise_body",2),("hold",0)],
    "spin":[("turn_right",180),("hold",0),("turn_left",90),("hold",0)],
}

def resolve_genre(raw):
    g=raw.strip().lower()
    if g in GENRE_POOLS: return g
    return GENRE_ALIASES.get(g,"pop")

def map_angle(move_name, raw_angle):
    info=ANGLE_TYPES.get(move_name,{"type":"intensity","default":10})
    angle=raw_angle if raw_angle else info["default"]
    t=info["type"]
    if t=="rotation": return max(10,min(360,int(round(angle*3))))
    elif t=="roll": return max(5,min(30,int(round(angle))))
    elif t=="pitch": return max(5,min(30,int(round(angle))))
    elif t=="yaw": return max(5,min(45,int(round(angle))))
    elif t=="height": return max(5,min(30,int(round(angle*2))))
    elif t=="duration": return max(100,min(5000,int(round(angle))))
    elif t=="intensity": return max(1,min(50,int(round(angle))))
    return int(round(angle))

def normalize_move_name(name):
    n=name.strip().lower().replace("-","_")
    if n in MOVE_ALIASES: return MOVE_ALIASES[n]
    if n=="step_move": return "step"
    return n

def expand_compounds(entries):
    expanded=[]; shift=0.0
    last_end=max(e["start_time"]+e.get("duration_ms",0)/1000.0 for e in entries) if entries else 0
    for entry in entries:
        cmd=normalize_move_name(entry["cmd"])
        dur=float(entry.get("duration_ms",250))/1000.0
        slot_dur=max(dur,0.1); adj_start=entry["start_time"]+shift
        if cmd in COMPOUND_EXPANSIONS:
            sub_moves=COMPOUND_EXPANSIONS[cmd]; n=len(sub_moves)
            for j,(sc,fp) in enumerate(sub_moves):
                expanded.append({"cmd":sc,"duration_ms":int(slot_dur*1000),"param":fp,"start_time":adj_start+j*slot_dur})
            shift+=(n-1)*slot_dur
        else:
            e2=dict(entry); e2["start_time"]=adj_start; expanded.append(e2)
    return [e for e in expanded if e["start_time"]<last_end]

def convert_timing(entries):
    if not entries: return []
    converted=[]; prev_end=0.0
    for entry in entries:
        start=entry["start_time"]
        dur_ms=entry.get("duration_ms",250)
        delay_ms=max(0,int(round((start-prev_end)*1000)))
        cmd=normalize_move_name(entry["cmd"])
        mid=MOVE_IDS.get(cmd,0)
        param=entry.get("param",0)
        if param==0 or isinstance(param,float): param=map_angle(cmd,param)
        converted.append({"cmd_id":mid,"cmd_name":cmd,"param":param,"delay_ms":delay_ms,"start_time":start,"duration_ms":dur_ms})
        prev_end=start
    return converted

def generate_c_source(entries, meta):
    n=len(entries)
    total_dur=int(meta.get("duration_sec",180)*1000)
    if entries: total_dur=max(total_dur,int((entries[-1]["start_time"]+entries[-1]["duration_ms"]/1000.0)*1000))
    lines=["/* GENERATED BY mpx-dance-codegen */","/* Song: %s */"%meta.get("song","?"),"/* Genre: %s */"%meta.get("genre","?"),"/* BPM: %s */"%meta.get("bpm","?"),"/* Duration: %ss */"%meta.get("duration_sec","?"),"","#ifndef TIMETABLE_H","#define TIMETABLE_H",'#include "moves.h"',"","#define TIMETABLE_ENTRIES %d"%n,"#define TIMETABLE_DURATION_MS %d"%total_dur,"","const timetable_entry_t TIMETABLE[%d] = {"%n]
    for i,e in enumerate(entries):
        comma="," if i<n-1 else " "; lines.append("    {.cmd=%3d,.param=%4d,.delay_ms=%5d}%s// %s @ t=%.1fs"%(e["cmd_id"],e["param"],e["delay_ms"],comma,e["cmd_name"]+(" "*(25-len(e["cmd_name"]))),e["start_time"]))
    lines.extend(["};","","#endif"])
    return "\n".join(lines)+"\n"

def generate_dance_wrapper(song_name):
    safe="".join(c for c in song_name.lower().replace(" ","_") if c.isalnum() or c=="_")
    return '/* GENERATED wrapper for %s */\n#include "timetable.h"\n#include "dance_runner.c"\n'%song_name

def select_genre_moves(beat_slots, genre, song_name):
    canonical=resolve_genre(genre)
    pool=GENRE_POOLS.get(canonical,GENRE_POOLS["pop"])
    seed_int=int(hashlib.sha256(song_name.encode()).hexdigest(),16)
    rng=random.Random(seed_int)
    moves=[]
    for slot in beat_slots:
        start=slot.get("start_time",0)
        ta=slot.get("time_acc",0.5)
        lbpm=slot.get("local_bpm",120)
        dur_ms=max(100,int(ta*1000)) if ta>0 else 250
        move=normalize_move_name(rng.choices(pool["moves"],weights=pool["weights"],k=1)[0])
        raw_angle=min(100,max(1,int(lbpm/5))) if lbpm else 50
        moves.append({"cmd":move,"duration_ms":dur_ms,"param":raw_angle,"start_time":start})
    return moves

def load_json(path):
    with open(path) as f: return json.load(f)

def cmd_generate(args):
    data=load_json(args.input)
    slots=data.get("beat_slots",data.get("data",{}).get("beat_slots",[]))
    bpm=data.get("bpm",data.get("data",{}).get("bpm",120))
    dur=data.get("duration_sec",data.get("data",{}).get("duration_sec",180))
    song=args.song or Path(args.input).stem
    genre=args.genre or "pop"
    print("🎵 %s  Genre: %s (%s)  BPM: %s  Duration: %ss  Slots: %d"%(song,genre,resolve_genre(genre),bpm,dur,len(slots)))
    print("🔄 Selecting genre moves...")
    moves=select_genre_moves(slots,genre,song)
    print("   → %d move slots"%len(moves))
    print("🔄 Expanding compounds...")
    exp=expand_compounds(moves)
    print("   → %d atomic moves"%len(exp))
    print("🔄 Converting timing...")
    conv=convert_timing(exp)
    safe="".join(c for c in song.lower().replace(" ","_") if c.isalnum() or c=="_")
    meta={"song":song,"genre":resolve_genre(genre),"bpm":bpm,"duration_sec":dur}
    c_code=generate_c_source(conv,meta)
    out_dir=Path(args.output) if args.output else Path(".")
    src_dir=out_dir/"src"; src_dir.mkdir(parents=True,exist_ok=True)
    (src_dir/"timetable.h").write_text(c_code)
    (src_dir/("dance_%s.c"%safe)).write_text(generate_dance_wrapper(song))
    enriched={"meta":meta,"moves":exp}
    with open(src_dir/("dance_%s.json"%safe),"w") as f: json.dump(enriched,f,indent=2)
    print("  📄 Wrote src/timetable.h, src/dance_%s.c, src/dance_%s.json"%(safe,safe))
    print("✅ Generated!  cd %s && mpx-cli build src/dance_%s.c && mpx-cli upload && mpx-cli run"%(out_dir,safe))
    cmd_counts={}; ee=conv
    for e in ee: cmd_counts[e["cmd_name"]]=cmd_counts.get(e["cmd_name"],0)+1
    print("📊 %d entries, ~%d KB WASM"%(len(conv),(len(conv)*5+4096)//1024))

def cmd_compile(args):
    data=load_json(args.input)
    meta=data.get("meta",{}); raw=data.get("moves",data.get("choreography",[]))
    song=meta.get("song",Path(args.input).stem)
    print("🎵 Compiling '%s' (%d moves)..."%(song,len(raw)))
    exp=expand_compounds(raw)
    print("   → %d atomic moves"%len(exp))
    conv=convert_timing(exp)
    meta.setdefault("genre",args.genre or "pop")
    meta.setdefault("bpm",120)
    meta.setdefault("duration_sec",conv[-1]["start_time"]+2 if conv else 180)
    safe="".join(c for c in song.lower().replace(" ","_") if c.isalnum() or c=="_")
    c_code=generate_c_source(conv,meta)
    out_dir=Path(args.output) if args.output else Path(".")
    src_dir=out_dir/"src"; src_dir.mkdir(parents=True,exist_ok=True)
    (src_dir/"timetable.h").write_text(c_code)
    (src_dir/("dance_%s.c"%safe)).write_text(generate_dance_wrapper(song))
    print("  📄 Wrote src/timetable.h, src/dance_%s.c"%safe)
    print("✅ cd %s && mpx-cli build src/dance_%s.c && mpx-cli upload src/dance_%s.wasm && mpx-cli run %s.wasm"%(out_dir,safe,safe,safe))

def cmd_expand(args):
    data=load_json(args.input); raw=data.get("moves",data.get("choreography",[]))
    if not raw:
        slots=data.get("beat_slots",[])
        if slots: raw=select_genre_moves(slots,args.genre or "pop",args.name or Path(args.input).stem)
    exp=expand_compounds(raw)
    out={"meta":data.get("meta",{}),"moves":exp}
    print(json.dumps(out,indent=2)); print("\n%d→%d after expansion"%(len(raw),len(exp)))

def cmd_stats(args):
    data=load_json(args.input); meta=data.get("meta",{}); moves=data.get("moves",data.get("choreography",[]))
    if not moves:
        slots=data.get("beat_slots",[])
        print("📊 Beat slots: %d"%(len(slots)))
        if slots: print("   Range: %.1fs-%.1fs"%(min(s["start_time"]for s in slots),max(s["start_time"]for s in slots)))
        return
    print("📊 Song: %s  Genre: %s  BPM: %s  Duration: %ss  Moves: %d"%(
        meta.get("song","?"),meta.get("genre","?"),meta.get("bpm","?"),meta.get("duration_sec","?"),len(moves)))
    cmd_counts={}
    for m in moves: c=m.get("cmd","?"); cmd_counts[c]=cmd_counts.get(c,0)+1
    for n,c in sorted(cmd_counts.items(),key=lambda x:-x[1]):
        print("   %s × %d (%d%%)"%(n+" "*(25-len(n)),c,int(c/len(moves)*100)))

def main():
    p=argparse.ArgumentParser(prog="codegen.py",description="MPX-Dance JSON→C Generator")
    sub=p.add_subparsers(dest="cmd",required=True)
    g=sub.add_parser("generate"); g.add_argument("input"); g.add_argument("--genre","-g",default="pop"); g.add_argument("--song","-s"); g.add_argument("--output","-o")
    c=sub.add_parser("compile"); c.add_argument("input"); c.add_argument("--genre","-g"); c.add_argument("--output","-o")
    e=sub.add_parser("expand"); e.add_argument("input"); e.add_argument("--genre","-g",default="pop"); e.add_argument("--name","-n")
    s=sub.add_parser("stats"); s.add_argument("input")
    a=p.parse_args()
    {"generate":cmd_generate,"compile":cmd_compile,"expand":cmd_expand,"stats":cmd_stats}[a.cmd](a)

if __name__=="__main__": main()
