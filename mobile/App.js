import React, { useEffect, useRef } from 'react';
import { View } from 'react-native';
import * as SplashScreen from 'expo-splash-screen';

SplashScreen.preventAutoHideAsync();
import { NavigationContainer } from '@react-navigation/native';
import { createNavigationContainerRef } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { createNativeStackNavigator } from '@react-navigation/native-stack';
import { SafeAreaProvider } from 'react-native-safe-area-context';
import { StatusBar } from 'expo-status-bar';
import * as Notifications from 'expo-notifications';
import * as Device from 'expo-device';
import Constants from 'expo-constants';
import { Platform } from 'react-native';
import { Ionicons } from '@expo/vector-icons';
import { Image } from 'react-native';

import HomeScreen from './screens/HomeScreen';
import IncidentsScreen from './screens/IncidentsScreen';
import IncidentDetailScreen from './screens/IncidentDetailScreen';
import LoginScreen from './screens/LoginScreen';
import { NotificationProvider, useNotifications } from './context/NotificationContext';
import { AuthProvider, useAuth } from './context/AuthContext';
import NotificationBell from './components/NotificationBell';
import { authFetch } from './utils/api';

Notifications.setNotificationHandler({
  handleNotification: async () => ({
    shouldShowAlert: true,
    shouldPlaySound: true,
    shouldSetBadge: true,
  }),
});

export const navigationRef = createNavigationContainerRef();

const Tab   = createBottomTabNavigator();
const Stack = createNativeStackNavigator();

function IncidentsStack() {
  return (
    <Stack.Navigator screenOptions={{ headerShown: false }}>
      <Stack.Screen name="IncidentsList"  component={IncidentsScreen} />
      <Stack.Screen name="IncidentDetail" component={IncidentDetailScreen} />
    </Stack.Navigator>
  );
}

function Tabs() {
  const { unreadCount } = useNotifications();
  const { logout } = useAuth();

  return (
    <Tab.Navigator
      screenOptions={{
        tabBarStyle: {
          backgroundColor: '#0D1117',
          borderTopColor: '#21262D',
          paddingBottom: 6,
          paddingTop: 4,
          height: 62,
        },
        tabBarActiveTintColor:   '#58A6FF',
        tabBarInactiveTintColor: '#8B949E',
        headerStyle:      { backgroundColor: '#0D1117' },
        headerTintColor:  '#E6EDF3',
        headerTitleStyle: { fontWeight: 'bold' },
        headerShadowVisible: false,
      }}
    >
      <Tab.Screen
        name="Home"
        component={HomeScreen}
        options={{
          tabBarLabel:  'Dashboard',
          tabBarIcon:   ({ color, size }) => <Ionicons name="grid-outline" size={size} color={color} />,
          headerTitle:  () => (
            <Image
              source={require('./assets/lens_logo.jpg')}
              style={{ width: 80, height: 40 }}
              resizeMode="contain"
            />
          ),
          headerRight:  () => (
            <View style={{ flexDirection: 'row', alignItems: 'center', marginRight: 8, gap: 12 }}>
              <NotificationBell />
              <Ionicons
                name="log-out-outline"
                size={24}
                color="#8B949E"
                onPress={logout}
              />
            </View>
          ),
        }}
      />
      <Tab.Screen
        name="Incidents"
        component={IncidentsStack}
        options={{
          title:          'Incidents',
          tabBarLabel:    'Incidents',
          headerShown:    false,
          tabBarIcon:     ({ color, size }) => <Ionicons name="warning-outline" size={size} color={color} />,
          tabBarBadge:    unreadCount > 0 ? unreadCount : undefined,
          tabBarBadgeStyle: { backgroundColor: '#FF4444', fontSize: 10 },
        }}
      />
    </Tab.Navigator>
  );
}

function AppNavigator() {
  const { token, loading } = useAuth();

  useEffect(() => {
    if (!loading) SplashScreen.hideAsync();
  }, [loading]);

  // Register push token with backend whenever user is logged in
  useEffect(() => {
    if (!token) return;
    registerAndSendPushToken();
  }, [token]);

  if (loading) return null;

  return (
    <NavigationContainer ref={navigationRef}>
      <StatusBar style="light" />
      {token ? (
        <NotificationProvider>
          <Tabs />
        </NotificationProvider>
      ) : (
        <LoginScreen />
      )}
    </NavigationContainer>
  );
}

export default function App() {
  const notifListener    = useRef();
  const responseListener = useRef();

  useEffect(() => {
    notifListener.current = Notifications.addNotificationReceivedListener(n => {
      console.log('Notification received:', n.request.content.title);
    });

    responseListener.current = Notifications.addNotificationResponseReceivedListener(r => {
      // Navigate to Incidents tab when user taps a push notification
      if (navigationRef.isReady()) {
        navigationRef.navigate('Incidents');
      }
    });

    return () => {
      notifListener.current?.remove();
      responseListener.current?.remove();
    };
  }, []);

  return (
    <SafeAreaProvider>
      <AuthProvider>
        <AppNavigator />
      </AuthProvider>
    </SafeAreaProvider>
  );
}

async function registerAndSendPushToken() {
  try {
    if (Platform.OS === 'android') {
      await Notifications.setNotificationChannelAsync('lens-alerts', {
        name:             'LENS Security Alerts',
        importance:       Notifications.AndroidImportance.MAX,
        vibrationPattern: [0, 250, 250, 250],
      });
    }

    if (!Device.isDevice) {
      console.warn('[LENS] Push tokens only work on physical devices');
      return;
    }

    const { status: existing } = await Notifications.getPermissionsAsync();
    let finalStatus = existing;
    if (existing !== 'granted') {
      const { status } = await Notifications.requestPermissionsAsync();
      finalStatus = status;
    }
    if (finalStatus !== 'granted') {
      console.warn('[LENS] Notification permission not granted');
      return;
    }

    const projectId =
      Constants.expoConfig?.extra?.eas?.projectId ??
      Constants.easConfig?.projectId;

    const { data: pushToken } = await Notifications.getExpoPushTokenAsync(
      projectId ? { projectId } : undefined
    );
    if (!pushToken) {
      console.warn('[LENS] No push token returned');
      return;
    }

    console.log('[LENS] Registering push token:', pushToken);
    const res = await authFetch('/push-token', {
      method: 'POST',
      body:   JSON.stringify({ token: pushToken }),
    });
    if (!res.ok) {
      const body = await res.text();
      console.warn('[LENS] Push token save failed:', res.status, body);
    } else {
      console.log('[LENS] Push token registered successfully');
    }
  } catch (err) {
    console.warn('[LENS] Push token registration error:', err.message);
  }
}
