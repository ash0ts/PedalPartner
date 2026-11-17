"""Service-oriented STEP hierarchy. Every physical part appears exactly once."""
from build123d import Compound

def category(name):
    if name.startswith(('A_', 'T_case_nut', 'U_tray_nut')):
        return '01_Chassis', 'Base_and_captive_nuts'
    if name.startswith(('H_', 'K_')):
        return '02_Front_Panel', 'Display_and_mounts'
    if name.startswith(('L_', 'P_')):
        return '02_Front_Panel', 'Finger_controls_and_LED'
    if name.startswith(('W_',)):
        return '02_Front_Panel', 'Cosmetic_inserts'
    if name.startswith(('B_', 'C_', 'D_', 'T_case_screw', 'V_')):
        return '02_Front_Panel', 'Lid_bezel_window_and_hardware'
    if name.startswith(('R_', 'S_')):
        return '03_Electronics_Carrier', 'Input_attenuator'
    if name.startswith(('F_', 'J_')):
        return '03_Electronics_Carrier', 'Controller_and_headers'
    if name.startswith(('G_', 'I_')):
        return '03_Electronics_Carrier', 'Audio_adapter_and_mounts'
    if name.startswith(('E_', 'U_tray_screw')):
        return '03_Electronics_Carrier', 'Tray_and_fixings'
    if name.startswith('Q_'):
        return '04_Rear_IO', 'Input_jack_and_retainer'
    if name.startswith(('M_', 'N_', 'O_')):
        return '05_Footswitch', 'Switch_load_plate_and_hardware'
    raise ValueError(f'Unassigned physical part: {name}')

def build_tree(parts, metadata):
    groups={}
    for name,part in parts.items():
        a,b=category(name)
        groups.setdefault(a,{}).setdefault(b,[]).append(part)
        metadata[name]['assembly_path']=f'{a}/{b}/{name}'
    nodes=[]; manifest={}
    for name,subgroups in sorted(groups.items()):
        children=[];manifest[name]={}
        for sub,items in subgroups.items():
            children.append(Compound(label=sub,children=items))
            manifest[name][sub]=[p.label for p in items]
        nodes.append(Compound(label=name,children=children))
    return Compound(label='VEX_Pocket_Rival_R1_1',children=nodes),manifest
