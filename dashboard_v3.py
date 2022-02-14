# from ctypes.wintypes import PCHAR
import json
from operator import index
import firebase_admin
from firebase_admin import db
from firebase_admin import credentials
import time
from encodings import normalize_encoding
import streamlit as st
import numpy as np
import pandas as pd

import plotly.express as px

# from dashboard.dashboard_v1 import DATA_URL

temperature = []
pressure = []
humidity = []
ph = []
turbidity = []
dates = []

# do this every time it is run on a new PC
# default_app = firebase_admin.initialize_app()

# Fetch the service account key JSON file contents
try:
    app = firebase_admin.get_app()
except ValueError as e:
    cred = credentials.Certificate('secrets.json')
    firebase_admin.initialize_app(cred, {
        'databaseURL': 'https://esp32-test-project-a01-default-rtdb.firebaseio.com/'
    })

ref = db.reference('/Nodes')
data = ref.get()

data_json = json.dumps(data)

for date in data["Node1"]:
    try:
        turb = data["Node1"][str(date)]["Turbidity"]
    except KeyError:
        turb = 0.0
    try:
        temp = data["Node1"][str(date)]["Temperature"]
    except KeyError:
        temp = 0.0
    try:
        humi = data["Node1"][str(date)]["Humidity"]
    except KeyError:
        humi = 0.0
    try:
        pressu = data["Node1"][str(date)]["Pressure"]
    except KeyError:
        pressu = 0.0
    try:
        ph_v = data["Node1"][str(date)]["PH"]
    except KeyError:
        ph_v = 0.0

    temperature.append(float(temp))
    humidity.append(float(humi))
    pressure.append(float(pressu))
    turbidity.append(float(turb))
    ph.append(float(ph_v))
    dates.append(str(date))


with open("realtime_data.json", "w") as f:
    json.dump(data, f, indent=2)


# st.header("Malnutrition Indicators in Tharparkar")

main_list = [dates, temperature, humidity, pressure, turbidity, ph]
main_dict = {}

for i, d in zip(range(len(dates)), dates):
    main_dict[d] = [temperature[i],
                    humidity[i],
                    pressure[i],
                    turbidity[i],
                    ph[i]]

# print(main_list)
# print(main_dict)

columns = ["Temperature", "Humidity", "Pressure", "Turbidity", "PH"]

# chart_data = pd.DataFrame([dates, temperature], columns=["Dates", "Temperature"], dtype="float")
chart_data = pd.DataFrame.from_dict(main_dict, orient='index', columns=columns)
chart_data.index.name = 'datetime'

# print(chart_data['Temperature'])

temp_data = chart_data['Temperature']
humi_data = chart_data['Humidity']
presse_data = chart_data['Pressure']
turb_data = chart_data['Turbidity']
ph_data = chart_data['PH']
st.header('Malnutrition Indicators in Tharparkar')

st.header('Node Location')
map_data = pd.DataFrame(
    [[24.93418524900444, 67.11216645686935]],
    columns=['lat', 'lon']
)
st.map(map_data)

st.header('Temperature')
st.metric('Last Updated Temperature', temp_data.iloc[-1], delta="{:.2f}".format(temp_data.iloc[1] - temp_data.iloc[2]))
fig = px.line(temp_data, y="Temperature")
fig.update_xaxes(rangeslider_visible = True)
fig.update_layout(width=1200,
                  height=600
                #   paper_bgcolor="#fff",
                #   plot_bgcolor="#fff"
                  )
fig.update_traces(line_color="#ff0000", line_width=5)

st.plotly_chart(fig, use_container_width=True)
# st.line_chart(temp_data, width=1200, height=400, use_container_width=True)

st.header('Humidity')
st.metric('Last Updated Humidity', humi_data.iloc[-1], delta="{:.2f}".format(humi_data.iloc[1] - humi_data.iloc[2]))
# st.line_chart(humi_data, width=1200, height=400, use_container_width=True)
fig = px.line(humi_data, y="Humidity")
fig.update_xaxes(rangeslider_visible = True)
fig.update_layout(width=1200,
                  height=600
                #   paper_bgcolor="#fff",
                #   plot_bgcolor="#fff"
                  )
fig.update_traces(line_color="#00ff00", line_width=5)

st.plotly_chart(fig, use_container_width=True)

st.header('Pressure')
st.metric('Last Updated Pressure', presse_data.iloc[-1], delta="{:.2f}".format(presse_data.iloc[1] - presse_data.iloc[2]))
# st.line_chart(presse_data, width=1200, height=400, use_container_width=True)
fig = px.line(presse_data, y="Pressure")
fig.update_xaxes(rangeslider_visible = True)
fig.update_layout(width=1200,
                  height=600
                #   paper_bgcolor="#fff",
                #   plot_bgcolor="#fff"
                  )
fig.update_traces(line_color="#0000ff", line_width=5)

st.plotly_chart(fig, use_container_width=True)


st.header('Turbidity')
st.metric('Last Updated Turbidity', turb_data.iloc[-1], delta="{:.2f}".format(turb_data.iloc[1] - turb_data.iloc[2]))
# st.line_chart(turb_data, width=1200, height=400, use_container_width=True)
fig = px.line(turb_data, y="Turbidity")
fig.update_xaxes(rangeslider_visible = True)
fig.update_layout(width=1200,
                  height=600
                #   paper_bgcolor="#fff",
                #   plot_bgcolor="#fff"
                  )
fig.update_traces(line_color="#ff0000", line_width=5)

st.plotly_chart(fig, use_container_width=True)

st.header('PH')
st.metric('Last Updated PH', ph_data.iloc[-1], delta="{:.2f}".format(ph_data.iloc[1] - ph_data.iloc[2]))
# st.line_chart(ph_data, width=1200, height=400, use_container_width=True)
fig = px.line(ph_data, y="PH")
fig.update_xaxes(rangeslider_visible = True)
fig.update_layout(width=1200,
                  height=600
                #   paper_bgcolor="#fff",
                #   plot_bgcolor="#fff"
                  )
fig.update_traces(line_color="#00ff00", line_width=5)

st.plotly_chart(fig, use_container_width=True)

