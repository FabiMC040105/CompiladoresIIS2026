#!/usr/bin/env python3
"""Casos negativos obligatorios para la reordenacion FEIST."""
import unittest
import empaquetar_vliw_feist as m


def program(lines):
    return [{op.slot:op for s in line.split('|') if (op:=m.parse_inst(s.strip()))} for line in lines]

class FeistSafety(unittest.TestCase):
    def test_fusion_cuatro(self):
        src=program(['FEIST: FROUND x13, x14, x5, x6, 0, 0, 0',
            'ALU: ADDI x11, x0, 9 | LSU: LOAD x12, 0(x0) | BRU: BNE x0, x0, 16',
            'ALU: NOP'])
        out,c=m.pack(src,m.targets(src,set()))
        self.assertEqual((c['FEIST_MIX'],c['CUATRO_SLOTS']),(1,1))
        self.assertEqual(set(out[1]),{'ALU','LSU','BRU','FEIST'})
    def test_target_bloquea(self):
        src=program(['FEIST: LOGOUT','ALU: ADDI x4, x0, 1'])
        _,c=m.pack(src,{1})
        self.assertEqual(c['FEIST_MIX'],0)
    def test_waw_bloquea(self):
        src=program(['FEIST: FROUND x4, x5, x8, x9, 0, 0, 0','ALU: ADDI x4, x0, 1'])
        _,c=m.pack(src,set())
        self.assertEqual(c['FEIST_MIX'],0)
    def test_raw_bloquea(self):
        src=program(['ALU: ADDI x8, x0, 7','FEIST: FROUND x4, x5, x8, x9, 0, 0, 0',
                    'ALU: ADDI x6, x0, 1'])
        _,c=m.pack(src,set())
        self.assertEqual(c['FEIST_MIX'],0)
    def test_consumidor_cercano_bloquea(self):
        src=program(['FEIST: FROUND x4, x5, x8, x9, 0, 0, 0',
                    'ALU: ADDI x6, x0, 1','ALU: ADD x7, x4, x0'])
        _,c=m.pack(src,set())
        self.assertEqual(c['FEIST_MIX'],0)
    def test_redireccion_pc_bloquea(self):
        src=program(['FEIST: LOGOUT','ALU: ADDI x1, x0, 16'])
        _,c=m.pack(src,set())
        self.assertEqual(c['FEIST_MIX'],0)
    def test_instruccion_feist_rechaza(self):
        with self.assertRaises(ValueError):
            m.parse_inst('FEIST: FROUND x13, x13, x5, x6, 0, 0, 0')

if __name__=='__main__':
    unittest.main(verbosity=2)
