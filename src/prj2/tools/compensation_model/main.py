import re

import matplotlib.pyplot as plt

class InputData():
    def __init__(self):
        self.total_points = 0
        self.input_min = 0
        self.input_max = 0
        self.raw_min = 0
        self.raw_max = 0
        self.raw_points = []

class RawPoint():
    def __init__(self):
        self.v_in = 0
        self.raw_out = 0

class Point():
    def __init__(self):
        self.p = 0;
        self.q = 0;
        self.ag = 0;
        self.bg = 0;
        self.range_begin = 0;
        self.range_end = 0;


DATA_FILE = "../../data/raw_data.dat"
OUTPUT_FILE = "comp.dat"


def load_data(filename):
    f = open(filename)
    data = f.read()
    f.close()

    data = re.sub('\s+', ' ', data).strip()
    data_array = data.split(' ')

    input_data = InputData()
    input_data.total_points = int(data_array[0])
    input_data.input_min = float(data_array[1])
    input_data.input_max = float(data_array[2])
    input_data.raw_min = int(data_array[3])
    input_data.raw_max = int(data_array[4])

    for e in data_array[5:]:
        p_arr = e.split(":")

        point = RawPoint()
        point.v_in = float(p_arr[0])
        point.raw_out = int(p_arr[1])

        input_data.raw_points += [point]


    return input_data


def compute_g(i_d):
    ret = []

    a = (i_d.raw_max - i_d.raw_min)/(i_d.input_max - i_d.input_min)

    for i in range(0, len(i_d.raw_points)-1):
        c = i_d.raw_points[i]
        n = i_d.raw_points[i+1]

        p = (n.raw_out-c.raw_out)/(n.v_in-c.v_in);
        q = c.raw_out - p*c.v_in
        ag = a-p
        bg = -q

        point = Point()
        point.p = p
        point.q = q
        point.ag = ag
        point.bg = bg
        point.range_begin = c.raw_out
        point.range_end = n.raw_out

        #cheat a little add one point to last range to make the search easier
        if i == len(i_d.raw_points)-2:
            point.range_end += 1

        ret += [point]

    return ret

def save_g(filename, g):
    f = open(filename, 'w')

    f.write(' ' + str(len(g)) + '\n') 
    for p in g:
        f.write(' ' + str(p.p) + ' ')
        f.write(' ' + str(p.q) + ' ')
        f.write(' ' + str(p.ag) + ' ')
        f.write(' ' + str(p.bg) + ' ')
        f.write(' ' + str(p.range_begin) + ' ')
        f.write(' ' + str(p.range_end) + '\n')

    f.close()

def check_range(point, val):
    #PORT TO C
    #0- in range
    #1- above
    #-1 below
    if val >= point.range_begin and val < point.range_end:
        return 0
    elif val < point.range_begin:
        return -1
    else: #val >= point.range_end
        return 1

def mid(b,e):
    #PORT TO C
    tmp = b+((e-b)/2)

    if tmp % 1 > 0:
        return (int(tmp) + 1)
    else:
        return int(tmp)

def search_by_raw_out(point_set, val):
    #PORT TO C
    #assuming correct data and in total range
    #Binary search by raw ADC value
    begin = 0
    end = len(point_set) -1

    while end != begin:
        m = mid(begin, end)
        m_p = point_set[m]
        pos = check_range(m_p, val)

        if pos == 0:
            return m
        elif pos == -1:
            end = m-1
        else: # pos == 1
            begin = m+1

    return begin

def compensate(g_set, f):
    #PORT TO C
    #find corresponding range
    idx_range = search_by_raw_out(g_set, f)
    r = g_set[idx_range]

    #use p and q to get original input
    v_in = (f - r.q)/r.p

    #compute g(x)
    g = v_in*r.ag + r.bg

    #return original added with g compensation
    return f+g

if __name__ == "__main__":
    input_data = load_data(DATA_FILE)
    g_func = compute_g(input_data)

    save_g(OUTPUT_FILE, g_func)



    #verify data
    ideal_a = (input_data.raw_max-input_data.raw_min)/(input_data.input_max-input_data.input_min)

    x_v_in = []
    y_out_raw = []
    y_out_comp = []
    y_out_ideal = []
    for p in input_data.raw_points:
        v_in = p.v_in
        out_raw = p.raw_out
        out_comp = compensate(g_func,out_raw)
        out_ideal = v_in*ideal_a

        print("IN:%f RAW:%d COMP: %f IDEAL: %f" %(v_in, out_raw, out_comp, out_ideal))

        x_v_in += [v_in]
        y_out_raw += [out_raw]
        y_out_comp += [out_comp]
        y_out_ideal += [out_ideal]

    plt.style.use('grayscale')
    plt.plot(x_v_in, y_out_raw, 'x', label="raw")
    plt.plot(x_v_in, y_out_ideal, '-', linewidth=0.7, label="ideal")
    plt.xlabel('input VDC')
    plt.ylabel('ADC output')
    plt.title('ADC raw data')
    plt.legend()
    plt.show()

    plt.style.use('grayscale')
    plt.plot(x_v_in, y_out_comp, '.', label="compensated")
    plt.plot(x_v_in, y_out_ideal, '-', linewidth=0.7, label="ideal")
    plt.xlabel('input VDC')
    plt.ylabel('ADC output')
    plt.title('ADC compensated data')
    plt.legend()
    plt.show()

    plt.style.use('grayscale')
    plt.plot(x_v_in, y_out_raw, 'x', label="raw")
    plt.plot(x_v_in, y_out_comp, '.', label="compensated")
    plt.xlabel('input VDC')
    plt.ylabel('ADC output')
    plt.title('ADC raw and compensated')
    plt.legend()
    plt.show()


        
