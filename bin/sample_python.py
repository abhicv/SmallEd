#NOTE(abhicv): created for the purpose of semester V EEE Machines II assignment
# Calculates the % voltage regulations for different pf of a salient pole alternator

import math
import cmath

#NOTE(abhicv): values to be changed(alternator properties)
Xd = 1.0 #direct axis reactance
Xq = 0.6 #quadrature axis reactance
R = 0.03 # armature resistance
Mod_I = 0.50 #current rms value(or per unit value)
V = 1.0 # terminal voltage rms value(or per unit value)

cos_phis = [0.0, 0.2, 0.4, 0.6, 0.8, 1.0]

#for lagging pf
print("Lagging")
for cos_phi in cos_phis:
    phi = cmath.acos(cos_phi)
    I = Mod_I * complex(cos_phi, -cmath.sin(phi))
    Eq = V + I * complex(R, Xq)
    delta = cmath.polar(Eq)[1]

    Mod_Iq = Mod_I * cmath.cos(delta + phi)
    Mod_Id = Mod_I * cmath.sin(delta + phi)

    Iq = Mod_Iq * complex(cmath.cos(delta), cmath.sin(delta))
    Id = Mod_Id * complex(cmath.cos((cmath.pi / 2.0) - delta), -cmath.sin((cmath.pi / 2.0) - delta))

    E = V + (I * R) + (Id * Xd) * 1j + (Iq * Xq) * 1j

    E = V + (I * R) + ((Id * Xd) * 1j) + ((Iq * Xq) * 1j)

    a = ((Id * Xd) * 1j)
    b = ((Iq * Xq) * 1j)
    c = I * R

    print("# cos(phi) = " + str(cos_phi)
        + " |E| = " + str(cmath.polar(E)[0])
        + " delta = " + str(math.degrees(cmath.polar(E)[1]))
        + " U = " + str((cmath.polar(E)[0] - V) * 100 / V) + " %")

    #print("j(id * xd) = " + str(cmath.polar(a)[0]), " angle = " + str(math.degrees(cmath.polar(a)[1])))
    #print("j(iq * xq) = " + str(cmath.polar(b)[0]), " angle = " + str(math.degrees(cmath.polar(b)[1])))
    #print("|i * r| = " + str(cmath.polar(c)[0]), " angle = " + str(math.degrees(cmath.polar(c)[1])))
    print("|v| = " + str(cmath.polar(V)[0]), " angle = " + str(math.degrees(cmath.polar(V)[1])))
    print("|Eq| = " + str(cmath.polar(Eq)[0]), " angle = " + str(math.degrees(cmath.polar(Eq)[1])))
    print("|Id| = " + str(cmath.polar(Id)[0]), " angle = " + str(math.degrees(cmath.polar(Id)[1])))
    print("|Iq| = " + str(cmath.polar(Iq)[0]), " angle = " + str(math.degrees(cmath.polar(Iq)[1])))

print("Leading")
for cos_phi in cos_phis:
    phi = cmath.acos(cos_phi)
    I = Mod_I * complex(cos_phi, cmath.sin(phi))
    Eq = V + I * complex(R, Xq)
    delta = cmath.polar(Eq)[1]

    Mod_Iq = Mod_I * cmath.cos(phi - delta)
    Mod_Id = Mod_I * cmath.sin(phi - delta)

    Iq = Mod_Iq * complex(cmath.cos(delta), cmath.sin(delta))
    Id = Mod_Id * complex(cmath.cos((cmath.pi / 2.0) + delta), cmath.sin((cmath.pi / 2.0) + delta))

    E = V + (I * R) + ((Id * Xd) * 1j) + ((Iq * Xq) * 1j)

    a = ((Id * Xd) * 1j)
    b = ((Iq * Xq) * 1j)
    c = I * R

    print("# cos(phi) = " + str(cos_phi)
        + " |E| = " + str(cmath.polar(E)[0])
        + " delta = " + str(math.degrees(cmath.polar(E)[1]))
        + " U = " + str((cmath.polar(E)[0] - V) * 100 / V) + " %")

    #print("j(id * xd) = " + str(cmath.polar(a)[0]), " angle = " + str(math.degrees(cmath.polar(a)[1])))
    #Erint("j(iq * xq) = " + str(cmath.polar(b)[0]), " angle = " + str(math.degrees(cmath.polar(b)[1])))
    #print("|i * r| = " + str(cmath.polar(c)[0]), " angle = " + str(math.degrees(cmath.polar(c)[1])))
    print("|v| = " + str(cmath.polar(V)[0]), " angle = " + str(math.degrees(cmath.polar(V)[1])))
    print("|Eq| = " + str(cmath.polar(Eq)[0]), " angle = " + str(math.degrees(cmath.polar(Eq)[1])))
    print("|Id| = " + str(cmath.polar(Id)[0]), " angle = " + str(math.degrees(cmath.polar(Id)[1])))
    print("|Iq| = " + str(cmath.polar(Iq)[0]), " angle = " + str(math.degrees(cmath.polar(Iq)[1])))

