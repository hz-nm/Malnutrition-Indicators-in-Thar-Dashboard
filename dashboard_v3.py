# from ctypes.wintypes import PCHAR
import json
from operator import index
import resource
import boto3
import time
from encodings import normalize_encoding
import streamlit as st
import numpy as np
import pandas as pd

import plotly.express as px
from boto3.dynamodb.conditions import Key, Attr

# from dashboard.dashboard_v1 import DATA_URL
temperature = []
pressure = []
humidity = []
ph = []
turbidity = []
dates = []

# do this every time it is run on a new PC
aws_access_key = "AKIAZZZNWBLN3O3MH24V"
aws_secret_key = "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"

region = "us-east-1"

dynamo_db = boto3.resource('dynamodb', region_name = region,
                            aws_access_key_id = aws_access_key,
                            aws_secret_access_key = aws_secret_key)

table = dynamo_db.Table('NodeData')

# Fetch the service account key JSON file contents
# We use the following resource
# https://boto3.amazonaws.com/v1/documentation/api/latest/guide/dynamodb.html#querying-and-scanning
# get all the data of Node 1
response = table.query(
    KeyConditionExpression=Key('id').eq('1')
)

# Now the response will be a list within which we will have a json.
# For example,
# [{u'username': u'johndoe',
#   u'first_name': u'John',
#   u'last_name': u'Doe',
#   u'account_type': u'standard_user',
#   u'age': Decimal('25'),
#   u'address': {u'city': u'Los Angeles',
#                u'state': u'CA',
#                u'zipcode': Decimal('90001'),
#                u'road': u'1 Jefferson Street'}}]

all_data = response['Items']
data = []

for i in range(len(all_data)):
    # get the temperature, pressure, humidity, pH and Turbidity
    turb = all_data[i]['Turbidity']
    temp = all_data[i]['Temperature']
    humi = all_data[i]['Humidity']
    pressu = all_data[i]['Pressure']
    ph_v = all_data[i]['PH']
    date = all_data[i]['TakenAt']

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
fig = px.line(temp_data, y="Temperature", markers=True)
fig.update_xaxes(rangeslider_visible = True)
fig.update_layout(width=1200,
                  height=600
                #   paper_bgcolor="#fff",
                #   plot_bgcolor="#fff"
                  )
fig.update_traces(line_color="#ff0000", line_width=3)

st.plotly_chart(fig, use_container_width=True)
# st.line_chart(temp_data, width=1200, height=400, use_container_width=True)

st.header('Humidity')
st.metric('Last Updated Humidity', humi_data.iloc[-1], delta="{:.2f}".format(humi_data.iloc[1] - humi_data.iloc[2]))
# st.line_chart(humi_data, width=1200, height=400, use_container_width=True)
fig = px.line(humi_data, y="Humidity", markers=True)
fig.update_xaxes(rangeslider_visible = True)
fig.update_layout(width=1200,
                  height=600
                #   paper_bgcolor="#fff",
                #   plot_bgcolor="#fff"
                  )
fig.update_traces(line_color="#00ff00", line_width=3)

st.plotly_chart(fig, use_container_width=True)

st.header('Pressure')
st.metric('Last Updated Pressure', presse_data.iloc[-1], delta="{:.2f}".format(presse_data.iloc[1] - presse_data.iloc[2]))
# st.line_chart(presse_data, width=1200, height=400, use_container_width=True)
fig = px.line(presse_data, y="Pressure", markers=True)
fig.update_xaxes(rangeslider_visible = True)
fig.update_layout(width=1200,
                  height=600
                #   paper_bgcolor="#fff",
                #   plot_bgcolor="#fff"
                  )
fig.update_traces(line_color="#0000ff", line_width=3)

st.plotly_chart(fig, use_container_width=True)


st.header('Turbidity')
st.metric('Last Updated Turbidity', turb_data.iloc[-1], delta="{:.2f}".format(turb_data.iloc[1] - turb_data.iloc[2]))
# st.line_chart(turb_data, width=1200, height=400, use_container_width=True)
fig = px.line(turb_data, y="Turbidity", markers=True)
fig.update_xaxes(rangeslider_visible = True)
fig.update_layout(width=1200,
                  height=600
                #   paper_bgcolor="#fff",
                #   plot_bgcolor="#fff"
                  )
fig.update_traces(line_color="#ff0000", line_width=3)

st.plotly_chart(fig, use_container_width=True)

st.header('PH')
st.metric('Last Updated PH', ph_data.iloc[-1], delta="{:.2f}".format(ph_data.iloc[1] - ph_data.iloc[2]))
# st.line_chart(ph_data, width=1200, height=400, use_container_width=True)
fig = px.line(ph_data, y="PH", markers=True)
fig.update_xaxes(rangeslider_visible = True)
fig.update_layout(width=1200,
                  height=600
                #   paper_bgcolor="#fff",
                #   plot_bgcolor="#fff"
                  )
fig.update_traces(line_color="#00ff00", line_width=3)

st.plotly_chart(fig, use_container_width=True)

